#pragma once
#include "language.hpp"
namespace uma {
// Opt-in affine ownership. Legacy references retain their shared semantics.
// Borrow lifetimes end at lexical scope boundaries; borrowed references cannot escape.
struct OwnershipChecker {
    Program& p;
    struct Binding {string name,type,mode;bool moved=false;shared_ptr<Binding> root;};
    using B=shared_ptr<Binding>;
    vector<map<string,B>> scopes{1};bool allowMove=false,inFunction=false;set<string> ownedClasses;
    void consume(const E& e){bool saved=allowMove;allowMove=true;expression(e);allowMove=saved;}
    explicit OwnershipChecker(Program& p):p(p){}
    B find(const string& n){for(auto s=scopes.rbegin();s!=scopes.rend();++s){auto i=s->find(n);if(i!=s->end())return i->second;}return {};}
    [[noreturn]] void bad(const Token& t,const string& msg){fail(t,"propriedade",msg);}
    B base(const E& e){if(!e)return {};if(e->kind==Expr::Self || e->kind==Expr::Super)return find("@self");if(e->kind==Expr::Var)return find(e->name);if(e->kind==Expr::Index || e->kind==Expr::Get || e->kind==Expr::Borrow)return base(e->a);return {};}
    B origin(B b){return b && b->root?b->root:b;}
    pair<int,int> loans(B root){int readers=0,writers=0;for(auto& s:scopes)for(auto& kv:s){auto b=kv.second;if(b->root==root){if(b->mode=="borrow mut")++writers;else ++readers;}}return {readers,writers};}
    void access(B b,const Token& t,bool write=false){
        if(!b)return;
        if(b->moved)bad(t,"Uso depois de mover: "+b->name);
        if(write && (b->mode=="const" || b->mode=="borrow"))bad(t,"Referencia somente para leitura: "+b->name);
        auto root=origin(b);auto active=loans(root);
        if(b==root && (active.second || (write && active.first)))bad(t,"Acesso conflita com emprestimo ativo de "+root->name);
    }
    void escape(const E& e,const Token& t){auto b=base(e);if(b && (b->mode=="own" || b->root || b->mode.rfind("borrow",0)==0) && refType(p,e->type))bad(t,"Referencia com ownership nao pode criar alias ou escapar; use mover(...) ou borrow.");}
    void expression(const E& e,bool write=false){
        if(!e)return;
        if(e->kind==Expr::Self || e->kind==Expr::Super){access(find("@self"),e->token,write);return;}
        if(e->kind==Expr::Var){auto b=find(e->name);if(inFunction && scopes[0].count(e->name) && scopes[0].at(e->name)==b && (b->mode=="own" || b->root))bad(e->token,"Funcoes recebem ownership por parametros; nao capturam globais own.");access(b,e->token,write);return;}
        if(e->kind==Expr::Lambda){for(auto& a:e->args){expression(a);escape(a,a->token);}return;}
        if(e->kind==Expr::Move){
            if(!allowMove)bad(e->token,"mover so pode transferir para own ou parametro own.");
            if(e->a->kind!=Expr::Var)bad(e->token,"mover exige uma variavel own.");
            auto b=find(e->a->name);
            if(!b || b->mode!="own")bad(e->token,"mover exige uma variavel own.");
            access(b,e->token,true);
            b->moved=true;
            return;
        }
        if(e->kind==Expr::Borrow)bad(e->token,"& so pode inicializar borrow ou ser passado a parametro borrow.");
        if(e->kind==Expr::Assign){
            auto b=base(e->a);bool direct=e->a->kind==Expr::Var;
            if(direct && b && b->root)bad(e->token,"O nome de um emprestimo nao pode ser reatribuido.");
            if(direct && b && b->mode=="own"){
                auto active=loans(b);if(active.first || active.second)bad(e->token,"Reatribuicao durante emprestimo ativo.");
                if(e->token.text!="=" || (e->b->kind!=Expr::Move && e->b->kind!=Expr::New && e->b->kind!=Expr::Array))bad(e->token,"own recebe uma alocacao nova ou mover(...).");
                consume(e->b);b->moved=false;return;
            }
            expression(e->a,true);expression(e->b);escape(e->b,e->token);return;
        }
        if(e->kind==Expr::Call){
            F f;
            if(e->a->kind==Expr::Var && p.functions.count(e->a->name))f=p.functions.at(e->a->name);
            if(e->a->kind==Expr::Get){expression(e->a->a,true);f=member(p,e->a->a->type,e->a->name).method;}
            vector<pair<B,bool>> temporary;
            bool mutation=e->a->kind==Expr::Var && (e->a->name=="falcon" || e->a->name=="dober" || e->a->name=="ordenar");
            for(size_t i=0;i<e->args.size();++i){auto a=e->args[i];string mode=f && i<f->params.size()?f->params[i].mode:"";
                if(mode.rfind("borrow",0)==0){
                    if(a->kind!=Expr::Borrow || a->a->kind!=Expr::Var)bad(a->token,"Parametro borrow exige &variavel ou &mut variavel.");
                    auto b=find(a->a->name);if(!b || (b->mode!="own" && b->mode.rfind("borrow",0)!=0))bad(a->token,"Emprestimo exige own ou parametro borrow.");
                    bool mut=mode=="borrow mut";if(mut != (a->name=="mut"))bad(a->token,"Mutabilidade do emprestimo difere do parametro.");
                    access(b,a->token,mut);auto root=origin(b);auto active=loans(root);
                    if((mut && (active.first || active.second)) || (!mut && active.second))bad(a->token,"Emprestimos conflitantes.");
                    for(auto& loan:temporary)if(loan.first==root && (mut || loan.second))bad(a->token,"Argumentos criam emprestimos conflitantes.");
                    temporary.push_back({root,mut});
                }else if(mode=="own"){
                    if(a->kind!=Expr::Move && a->kind!=Expr::New && a->kind!=Expr::Array)bad(a->token,"Parametro own exige mover(...) ou alocacao nova.");
                    consume(a);
                }else{
                    expression(a,mutation && i==0);
                    // Inspection primitives don't retain references. User code may.
                    if(f || !builtins.count(e->a->name) || a->kind==Expr::Borrow)escape(a,a->token);
                    auto receiver=e->a->kind==Expr::Get?base(e->a->a):B{};
                    if(receiver && (receiver->mode=="own" || receiver->mode.rfind("borrow",0)==0) && refType(p,a->type))bad(a->token,"Metodo de objeto own/borrow nao recebe referencia compartilhada; passe dados escalares.");
                    if(mutation && i>0)escape(a,a->token);
                }
            }
            return;
        }
        if(e->kind==Expr::Index || e->kind==Expr::Get){expression(e->a,write);expression(e->b);return;}
        if(e->kind==Expr::Unary || e->kind==Expr::Postfix){expression(e->a,write || e->token.text=="++" || e->token.text=="--");return;}
        expression(e->a);expression(e->b);
        for(auto& a:e->args){expression(a);escape(a,a->token);}
    }
    using State=vector<pair<B,bool>>;
    State snapshot(){State out;for(auto& s:scopes)for(auto& kv:s)out.push_back({kv.second,kv.second->moved});return out;}
    void restore(const State& state){for(auto& kv:state)kv.first->moved=kv.second;}
    void statement(const S& s,bool scoped=true){
        if(!s)return;
        if(s->kind==Stmt::Block){scoped=scoped && s->mode!="group";if(scoped)scopes.emplace_back();for(auto& c:s->body)statement(c);if(scoped)scopes.pop_back();return;}
        if(s->kind==Stmt::Variable){
            auto b=make_shared<Binding>();b->name=s->name;b->type=s->type;b->mode=s->mode;
            if(s->mode=="own"){
                if(!refType(p,s->type))bad(s->token,"own exige array ou instancia de classe.");
                if(!s->expr || (s->expr->kind!=Expr::New && s->expr->kind!=Expr::Array && s->expr->kind!=Expr::Move))bad(s->token,"own exige alocacao nova ou mover(...).");
                if(s->expr->kind==Expr::New)for(auto& arg:s->expr->args)if(refType(p,arg->type))bad(arg->token,"Construtor de own recebe dados escalares; campos com ownership ainda nao sao suportados.");
                consume(s->expr);
            }else if(s->mode.rfind("borrow",0)==0){
                auto a=s->expr;if(!a || a->kind!=Expr::Borrow || a->a->kind!=Expr::Var)bad(s->token,"borrow exige &variavel; borrow mut exige &mut variavel.");
                auto source=find(a->a->name);if(!source || source->mode!="own")bad(s->token,"Emprestimos locais exigem uma origem own.");
                bool mut=s->mode=="borrow mut";if(mut!=(a->name=="mut"))bad(s->token,"Mutabilidade do emprestimo difere da declaracao.");access(source,a->token,mut);
                auto active=loans(source);if((mut && active.first) || active.second)bad(s->token,"Emprestimos conflitantes.");b->root=source;
            }else{expression(s->expr);if(s->expr)escape(s->expr,s->token);}
            scopes.back()[s->name]=b;return;
        }
        if(s->kind==Stmt::If || s->kind==Stmt::Try){
            expression(s->cond);auto before=snapshot();statement(s->yes);auto a=snapshot();restore(before);
            if(s->no){scopes.emplace_back();if(s->kind==Stmt::Try){auto b=make_shared<Binding>();b->name=s->name;b->type="wit";scopes.back()[s->name]=b;}statement(s->no,false);scopes.pop_back();}
            for(auto& kv:a)kv.first->moved=kv.first->moved || kv.second;
            statement(s->cleanup);
            return;
        }
        if(s->kind==Stmt::For || s->kind==Stmt::While){
            scopes.emplace_back();statement(s->init);expression(s->cond);auto before=snapshot();statement(s->yes);expression(s->update);
            for(auto& kv:before)if(!kv.second && kv.first->moved)bad(s->token,"Valor own movido em laco precisa ser reinicializado antes da proxima volta: "+kv.first->name);
            scopes.pop_back();return;
        }
        expression(s->expr);if(s->kind==Stmt::Return && s->expr)escape(s->expr,s->token);
    }
    void function(const F& f){
        scopes.resize(1);inFunction=true;scopes.emplace_back();if(ownedClasses.count(f->owner)){auto self=make_shared<Binding>();self->name="teio";self->type=f->owner;self->mode="borrow mut";scopes.back()["@self"]=self;}for(auto& par:f->params){auto b=make_shared<Binding>();b->name=par.name;b->type=par.type;b->mode=par.mode;if(!par.mode.empty() && !refType(p,par.type))bad(par.token,"Parametro own/borrow exige tipo de referencia.");scopes.back()[par.name]=b;}
        statement(f->body,false);scopes.resize(1);inFunction=false;
    }
    void run(){std::function<void(const S&)> scan=[&](const S& s){if(!s)return;if(s->mode=="own" && p.classes.count(s->type))ownedClasses.insert(s->type);for(auto& child:s->body)scan(child);scan(s->init);scan(s->yes);scan(s->no);scan(s->cleanup);};for(auto& s:p.statements)scan(s);for(auto& kv:p.functions){scan(kv.second->body);for(auto& a:kv.second->params)if(!a.mode.empty() && p.classes.count(a.type))ownedClasses.insert(a.type);}auto roots=ownedClasses;for(auto& n:p.classOrder)for(auto& root:roots)if(subtype(p,n,root) || subtype(p,root,n))ownedClasses.insert(n);for(auto& s:p.statements)statement(s);for(auto& n:p.functionOrder)function(p.functions.at(n));for(auto& n:p.classOrder){auto c=p.classes.at(n);if(c->ctor)function(c->ctor);for(auto& kv:c->methods)function(kv.second);}}
};
}

