#pragma once
#include "language.hpp"
namespace uma {
// Conservative interprocedural check of reads of globals during startup.
struct InitializationChecker {
    Program& p;set<string> globals;
    struct Uses {set<string> globals,calls;};
    map<string,Uses> uses;
    using Locals=vector<set<string>>;
    explicit InitializationChecker(Program& p):p(p){for(auto& s:p.statements)if(s->kind==Stmt::Variable)globals.insert(s->name);}
    static string key(const F& f){return f->owner.empty()?f->name:f->owner+"."+f->name;}
    bool local(const Locals& locals,const string& n){for(auto& scope:locals)if(scope.count(n))return true;return false;}
    void expr(const E& e,Uses& u,Locals& locals){
        if(!e)return;
        if(e->kind==Expr::Var && globals.count(e->name) && !local(locals,e->name))u.globals.insert(e->name);
        if(e->kind==Expr::New)u.calls.insert(e->name+".opera");
        if(e->kind==Expr::Call){
            if(e->a->namedFunction)u.calls.insert(e->a->name);
            else if(e->a->kind==Expr::Get){for(auto& n:p.classOrder)if(n==e->a->a->type || subtype(p,n,e->a->a->type)){auto m=member(p,n,e->a->name);if(m.method)u.calls.insert(key(m.method));}}
            else if(e->a->kind==Expr::Var){for(auto& s:p.statements)if(s->kind==Stmt::Variable && s->name==e->a->name && s->expr){if(s->expr->namedFunction)u.calls.insert(s->expr->name);if(s->expr->kind==Expr::Lambda)u.calls.insert(s->expr->name);}}
        }
        expr(e->a,u,locals);expr(e->b,u,locals);for(auto& a:e->args)expr(a,u,locals);
    }
    void stmt(const S& s,Uses& u,Locals& locals,bool scoped=true){
        if(!s)return;
        if(s->kind==Stmt::Block){bool enter=scoped && s->mode!="group";if(enter)locals.emplace_back();for(auto& c:s->body)stmt(c,u,locals);if(enter)locals.pop_back();return;}
        if(s->kind==Stmt::For || s->kind==Stmt::While){locals.emplace_back();stmt(s->init,u,locals);expr(s->cond,u,locals);stmt(s->yes,u,locals);expr(s->update,u,locals);locals.pop_back();return;}
        expr(s->expr,u,locals);expr(s->cond,u,locals);expr(s->update,u,locals);
        if(s->kind==Stmt::Variable)locals.back().insert(s->name);
        stmt(s->yes,u,locals);
        if(s->no){locals.emplace_back();if(s->kind==Stmt::Try)locals.back().insert(s->name);stmt(s->no,u,locals);locals.pop_back();}
        stmt(s->cleanup,u,locals);
    }
    void function(const F& f){Locals locals(1);for(auto& a:f->params)locals.back().insert(a.name);stmt(f->body,uses[key(f)],locals,false);}
    void run(){
        for(auto& n:p.functionOrder)function(p.functions.at(n));
        for(auto& n:p.classOrder){auto c=p.classes.at(n);for(auto& f:c->methods)function(f.second);if(c->ctor)function(c->ctor);auto& u=uses[n+".opera"];Locals locals(1);if(c->ctor)for(auto& a:c->ctor->params)locals.back().insert(a.name);if(!c->base.empty())u.calls.insert(c->base+".opera");for(auto& f:c->fields)expr(f.init,u,locals);if(c->ctor)for(auto& a:c->ctor->baseArgs)expr(a,u,locals);}
        bool changed=true;while(changed){changed=false;for(auto& kv:uses){auto dependencies=kv.second.calls;for(auto& call:dependencies){auto f=uses.find(call);if(f==uses.end())continue;auto before=kv.second.globals.size();kv.second.globals.insert(f->second.globals.begin(),f->second.globals.end());changed|=before!=kv.second.globals.size();}}}
        set<string> initialized;
        for(auto& s:p.statements){Uses u;Locals locals(1);if(s->kind==Stmt::Variable)expr(s->expr,u,locals);else stmt(s,u,locals);for(auto& call:u.calls){auto it=uses.find(call);if(it!=uses.end())u.globals.insert(it->second.globals.begin(),it->second.globals.end());}for(auto& name:u.globals)if(!initialized.count(name))fail(s->token,"tipo","Global ainda nao inicializada nesta chamada: "+name);if(s->kind==Stmt::Variable)initialized.insert(s->name);}
    }
};
}
