#pragma once
#include "language.hpp"

namespace uma {
// Instrucoes de uma maquina de pilha. O arquivo compilado nao contem a AST
// nem uma copia do texto-fonte para ser interpretada novamente.
enum class Op : uint8_t {
    Closure, LocalLoad, LocalRef, LocalDeclare, AddInt, SubInt, MulInt, LtInt, LeInt, GtInt, GeInt, EqInt, NeInt, Try, Throw, SignalBreak, SignalContinue, MoveVar, Append, Void, Int, Real, Text, Character, Bool, Null, Default, Load, RefVar,
    Self, Function, Member, RefField, Index, RefIndex, ReadRef, StoreRef,
    Mutate, Array, New, Unary, Binary, Call, Dup, Drop, Declare,
    Enter, Leave, Unwind, Jump, JumpFalse, JumpTrue, Return, Nop
};
const char* opName(Op op) {
    static const char* names[] = {
        "CLOSURE","LOCAL_LOAD","LOCAL_REF","LOCAL_DECLARE","ADD_INT","SUB_INT","MUL_INT","LT_INT","LE_INT","GT_INT","GE_INT","EQ_INT","NE_INT","TRY","THROW","SIGNAL_BREAK","SIGNAL_CONTINUE","MOVE_VAR","APPEND","VOID","INT","REAL","TEXT","CHAR","BOOL","NULL","DEFAULT","LOAD","REF_VAR",
        "SELF","FUNCTION","MEMBER","REF_FIELD","INDEX","REF_INDEX","READ_REF","STORE_REF",
        "MUTATE","ARRAY","NEW","UNARY","BINARY","CALL","DUP","DROP","DECLARE",
        "ENTER","LEAVE","UNWIND","JUMP","JUMP_FALSE","JUMP_TRUE","RETURN","NOP"
    };
    return names[static_cast<size_t>(op)];
}
struct Instruction {
    Op op = Op::Nop;
    int64_t a = 0;
    string x, y;
    int line = 1, col = 1;
    Token token() const { return {x, "op", line, col}; }
};
using Code = vector<Instruction>;
constexpr uint32_t noCode = 0xffffffffu;
struct ClassCode { vector<uint32_t> fields, baseArgs; uint32_t ctor = noCode; };
struct Module {
    Program program;
    string sourceName;
    vector<Code> codes;
    map<string, uint32_t> functions;
    map<string, ClassCode> classes;
    uint32_t entry = 0;
};
string functionKey(const F& f) { return f->owner.empty() ? f->name : f->owner + "." + f->name; }

struct Emitter {
    Code code;
    vector<Code>* chunks=nullptr;bool outlined=false;
    vector<map<string,int64_t>> bindings{1};shared_ptr<int64_t> nextSlot=make_shared<int64_t>(0);
    int64_t resolve(const string& n){for(auto it=bindings.rbegin();it!=bindings.rend();++it){auto i=it->find(n);if(i!=it->end())return i->second;}return -1;}
    void load(const E& e,bool reference){auto slot=resolve(e->name);emit(slot<0?(reference?Op::RefVar:Op::Load):(reference?Op::LocalRef:Op::LocalLoad),e->token,slot,e->name);}
    void parameters(const vector<Param>& ps){for(auto& p:ps){emit(Op::Load,p.token,0,p.name);auto slot=(*nextSlot)++;bindings.back()[p.name]=slot;emit(Op::LocalDeclare,p.token,slot,p.name,p.type);}}

    uint32_t outline(const S& s){Emitter e;e.chunks=chunks;e.outlined=true;e.bindings=bindings;e.nextSlot=nextSlot;e.stmt(s);auto c=e.finish();auto id=static_cast<uint32_t>(chunks->size());chunks->push_back(move(c));return id;}
    int scopes = 0;
    struct Loop { int scope; vector<size_t> breaks, continues; };
    vector<Loop> loops;
    size_t emit(Op op, const Token& t, int64_t a = 0, string x = "", string y = "") {
        code.push_back({op,a,move(x),move(y),t.line,t.col}); return code.size()-1;
    }
    void patch(size_t i, size_t target) { code.at(i).a = static_cast<int64_t>(target); }
    void ref(const E& e) {
        if (e->kind == Expr::Var) load(e,true);
        else if (e->kind == Expr::Get) { expr(e->a); emit(Op::RefField,e->token,0,e->name); }
        else { expr(e->a); expr(e->b); emit(Op::RefIndex,e->token); }
    }
    void expr(const E& e) {
        const Token& t = e->token;
        switch (e->kind) {
        case Expr::Literal:
            if (e->type == "speed") emit(Op::Int,t,stoll(t.text));
            else if (e->type == "stamina") {
                double d = stod(t.text); int64_t bits; static_assert(sizeof(bits)==sizeof(d),"double precisa ter 64 bits");
                memcpy(&bits,&d,sizeof(bits)); emit(Op::Real,t,bits);
            } else if (e->type == "nature") emit(Op::Null,t);
            else if (e->type == "track") emit(Op::Bool,t,t.text=="turf");
            else emit(e->type=="guts" ? Op::Character : Op::Text,t,0,t.text);
            break;
        case Expr::Var:if(builtins.count(e->name) || e->namedFunction)emit(Op::Function,t,0,e->name);else load(e,false);break;
        case Expr::Lambda:for(auto& a:e->args)expr(a);emit(Op::Closure,t,static_cast<int64_t>(e->args.size()),e->name);break;
        case Expr::Borrow:expr(e->a);break;
        case Expr::Move:ref(e->a);emit(Op::MoveVar,t);break;
        case Expr::Self: case Expr::Super: emit(Op::Self,t); break;
        case Expr::Array:
            for (const auto& a : e->args) expr(a);
            emit(Op::Array,t,static_cast<int64_t>(e->args.size()),elementType(e->type)); break;
        case Expr::New:
            for (const auto& a : e->args) expr(a);
            emit(Op::New,t,static_cast<int64_t>(e->args.size()),e->name); break;
        case Expr::Unary:
            if (t.text=="++" || t.text=="--") { ref(e->a); emit(Op::Mutate,t,0,t.text); }
            else if (t.text=="-" && e->a->kind==Expr::Literal && e->a->token.text=="9223372036854775808") emit(Op::Int,t,numeric_limits<int64_t>::min());
            else { expr(e->a); emit(Op::Unary,t,0,t.text); }
            break;
        case Expr::Postfix: ref(e->a); emit(Op::Mutate,t,1,t.text); break;
        case Expr::Binary:
            expr(e->a);
            if (t.text=="&&" || t.text=="||") {
                emit(Op::Dup,t); size_t j=emit(t.text=="&&" ? Op::JumpFalse : Op::JumpTrue,t);
                emit(Op::Drop,t); expr(e->b); patch(j,code.size());
            } else {expr(e->b);static const map<string,Op> ops={{"+",Op::AddInt},{"-",Op::SubInt},{"*",Op::MulInt},{"<",Op::LtInt},{"<=",Op::LeInt},{">",Op::GtInt},{">=",Op::GeInt},{"==",Op::EqInt},{"!=",Op::NeInt}};auto it=ops.find(t.text);emit(e->a->type=="speed" && e->b->type=="speed" && it!=ops.end()?it->second:Op::Binary,t,0,t.text);}
            break;
        case Expr::Assign:
            if(t.text=="+=" && e->type=="wit" && (e->b->kind==Expr::Literal || e->b->kind==Expr::Var)){ref(e->a);expr(e->b);emit(Op::Append,t);break;}
            ref(e->a); if (t.text!="=") emit(Op::ReadRef,t);
            expr(e->b); if (t.text!="=") emit(Op::Binary,t,0,t.text.substr(0,t.text.size()-1));
            emit(Op::StoreRef,t); break;
        case Expr::Get: expr(e->a); emit(Op::Member,t,e->a->kind==Expr::Super,e->name); break;
        case Expr::Index: expr(e->a); expr(e->b); emit(Op::Index,t); break;
        case Expr::Call:
            // O checker nao precisa inferir o callee inteiro para uma chamada.
            if(e->a->kind==Expr::Var && (e->a->namedFunction || builtins.count(e->a->name)))emit(Op::Function,e->a->token,0,e->a->name);
            else expr(e->a);
            for (const auto& a : e->args) expr(a);
            emit(Op::Call,t,static_cast<int64_t>(e->args.size())); break;
        }
    }
    void stmt(const S& s, bool scoped = true) {
        const Token& t=s->token;
        switch (s->kind) {
        case Stmt::Block:
            scoped=scoped && s->mode!="group";
            if (scoped) { emit(Op::Enter,t); ++scopes;bindings.emplace_back(); }
            for (const auto& child : s->body) stmt(child);
            if (scoped) { emit(Op::Leave,t); --scopes;bindings.pop_back(); }
            break;
        case Stmt::Variable:
            if (s->expr) expr(s->expr); else emit(Op::Default,t,0,s->type);
            if(scopes || outlined || !bindings[0].empty()){auto slot=(*nextSlot)++;bindings.back()[s->name]=slot;emit(Op::LocalDeclare,t,slot,s->name,s->type);}else emit(Op::Declare,t,0,s->name,s->type);break;
        case Stmt::Expression: if (s->expr) { expr(s->expr); emit(Op::Drop,t); } break;
        case Stmt::If: {
            expr(s->cond); size_t no=emit(Op::JumpFalse,t); stmt(s->yes);
            size_t end=emit(Op::Jump,t); patch(no,code.size()); if (s->no) stmt(s->no); patch(end,code.size()); break;
        }
        case Stmt::While: case Stmt::For: {
            emit(Op::Enter,t); ++scopes;bindings.emplace_back();
            if (s->init) stmt(s->init);
            size_t begin=code.size();
            if (s->cond) expr(s->cond); else emit(Op::Bool,t,1);
            size_t end=emit(Op::JumpFalse,t);
            loops.push_back({scopes,{},{}}); stmt(s->yes);
            size_t update=code.size();
            for(size_t ii=begin;ii<code.size();++ii)if(auto& ins=code[ii];ins.op==Op::Try && ins.y.find("|")==string::npos)ins.y+="|"+to_string(update)+"|"+to_string(scopes);
            if (s->update) { expr(s->update); emit(Op::Drop,t); }
            emit(Op::Jump,t,static_cast<int64_t>(begin));
            for(auto& ins:code)if(ins.op==Op::Try && ins.y.find("|end=")==string::npos && ins.y.find("|")!=string::npos)ins.y+="|end="+to_string(code.size());
            patch(end,code.size()); for (auto j : loops.back().breaks) patch(j,code.size());
            for (auto j : loops.back().continues) patch(j,update);
            loops.pop_back(); emit(Op::Leave,t); --scopes;bindings.pop_back(); break;
        }
        case Stmt::ForEach:throw logic_error("foreach nao foi resolvido");
        case Stmt::Throw:expr(s->expr);emit(Op::Throw,t);break;
        case Stmt::Try:{auto body=outline(s->yes);string handler=s->no?to_string(outline(s->no)):"-";string cleanup=s->cleanup?to_string(outline(s->cleanup)):"-";emit(Op::Try,t,body,handler+":"+cleanup,s->name);break;}
        case Stmt::Return:
            if (s->expr) expr(s->expr); else emit(Op::Void,t);
            emit(Op::Return,t,1); break;
        case Stmt::Break: case Stmt::Continue: {
            if(loops.empty() && outlined){emit(s->kind==Stmt::Break?Op::SignalBreak:Op::SignalContinue,t);break;}
            emit(Op::Unwind,t,scopes-loops.back().scope);
            size_t j=emit(Op::Jump,t);
            if (s->kind==Stmt::Break) loops.back().breaks.push_back(j); else loops.back().continues.push_back(j);
            break;
        }
        }
    }
    Code finish() { Token t; emit(Op::Void,t); emit(Op::Return,t); return move(code); }
};

Module compile(Program p, const string& path) {
    Module m; m.program=move(p); m.sourceName=filesystem::u8path(path).filename().u8string();
    auto add=[&](Emitter e) { uint32_t n=static_cast<uint32_t>(m.codes.size()); m.codes.push_back(e.finish()); return n; };
    auto expression=[&](const E& x) { Emitter e;e.chunks=&m.codes; e.expr(x); e.emit(Op::Return,x->token); return add(move(e)); };
    Emitter entry;entry.chunks=&m.codes; for (const auto& s : m.program.statements) entry.stmt(s); m.entry=add(move(entry));
    auto fn=[&](const F& f) { Emitter e;e.chunks=&m.codes;e.parameters(f->params);if(f->params.empty())e.bindings[0]["@frame"]=-1; e.stmt(f->body,false); return add(move(e)); };
    for (const auto& n : m.program.functionOrder) { auto f=m.program.functions.at(n); m.functions[functionKey(f)]=fn(f); }
    for (const auto& n : m.program.classOrder) {
        auto c=m.program.classes.at(n); ClassCode cc;
        for (const auto& f : c->fields) cc.fields.push_back(f.init ? expression(f.init) : noCode);
        if (c->ctor) { for (const auto& e : c->ctor->baseArgs) cc.baseArgs.push_back(expression(e)); cc.ctor=fn(c->ctor); }
        vector<string> names; for (const auto& x : c->methods) names.push_back(x.first); sort(names.begin(),names.end());
        for (const auto& name : names) { auto f=c->methods.at(name); m.functions[functionKey(f)]=fn(f); }
        m.classes[n]=move(cc);
    }
    // A VM recebe apenas assinaturas e instrucoes, mesmo no comando run.
    m.program.statements.clear();
    for(auto& pair:m.program.functions)pair.second->body.reset();
    for(auto& pair:m.program.classes){
        for(auto& f:pair.second->fields)f.init.reset();
        if(pair.second->ctor){pair.second->ctor->body.reset();pair.second->ctor->baseArgs.clear();}
        for(auto& f:pair.second->methods)f.second->body.reset();
    }
    return m;
}

// Formato binario explicito little-endian, versionado e com CRC-32.
struct Writer {
    string bytes;
    void u8(uint8_t n) { bytes+=static_cast<char>(n); }
    void u32(uint32_t n) { for (int k=0;k<4;++k) u8(static_cast<uint8_t>(n>>(8*k))); }
    void u64(uint64_t n) { for (int k=0;k<8;++k) u8(static_cast<uint8_t>(n>>(8*k))); }
    void str(const string& s) { if (s.size()>1048576) throw invalid_argument("Texto excede 1 MiB no bytecode."); u32(static_cast<uint32_t>(s.size())); bytes+=s; }
};
uint32_t crc32(const string& data) {
    uint32_t crc=0xffffffffu;
    for (unsigned char c : data) { crc^=c; for (int i=0;i<8;++i) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u))); }
    return ~crc;
}
struct Reader {
    const string& bytes; size_t pos=0;
    explicit Reader(const string& b) : bytes(b) {}
    [[noreturn]] void bad() const { throw invalid_argument("Bytecode invalido, truncado ou incompativel."); }
    uint8_t u8() { if (pos>=bytes.size()) bad(); return static_cast<uint8_t>(bytes[pos++]); }
    uint32_t u32() { uint32_t n=0; for (int k=0;k<4;++k) n|=uint32_t(u8())<<(8*k); return n; }
    uint64_t u64() { uint64_t n=0; for (int k=0;k<8;++k) n|=uint64_t(u8())<<(8*k); return n; }
    uint32_t count(uint32_t max=1000000) { auto n=u32(); if (n>max || n>bytes.size()-pos) bad(); return n; }
    string str() { uint32_t n=count(1048576); string s=bytes.substr(pos,n); pos+=n; return s; }
};
void writeSignature(Writer& w, const F& f) {
    w.str(f->name); w.str(f->result); w.str(f->owner); w.u8(f->priv); w.u32(static_cast<uint32_t>(f->token.line));
    w.u32(static_cast<uint32_t>(f->params.size())); for (const auto& a : f->params) { w.str(a.type); w.str(a.name); }
}
F readSignature(Reader& r) {
    auto f=make_shared<Function>(); f->name=r.str(); f->result=r.str(); f->owner=r.str(); f->priv=r.u8()!=0;
    f->token={f->name,"id",static_cast<int>(r.u32()),1}; uint32_t n=r.count(4096);
    for (uint32_t i=0;i<n;++i) { string ty=r.str(), name=r.str(); f->params.push_back({move(ty),move(name),f->token,""}); }
    return f;
}
string serialize(const Module& m) {
    Writer w; w.bytes="UMABC03\n"; w.u32(3); w.str(m.sourceName); w.u32(m.entry);
    w.u32(static_cast<uint32_t>(m.codes.size()));
    for (const auto& c : m.codes) {
        w.u32(static_cast<uint32_t>(c.size()));
        for (const auto& i : c) { w.u8(static_cast<uint8_t>(i.op)); uint64_t bits; memcpy(&bits,&i.a,8); w.u64(bits); w.str(i.x); w.str(i.y); w.u32(static_cast<uint32_t>(i.line)); w.u32(static_cast<uint32_t>(i.col)); }
    }
    w.u32(static_cast<uint32_t>(m.program.functionOrder.size()));
    for (const auto& n : m.program.functionOrder) { auto f=m.program.functions.at(n); writeSignature(w,f); w.u32(m.functions.at(functionKey(f))); }
    w.u32(static_cast<uint32_t>(m.program.interfaces.size()));
    vector<string> interfaces;for(auto& kv:m.program.interfaces)interfaces.push_back(kv.first);sort(interfaces.begin(),interfaces.end());
    for(auto& name:interfaces){auto c=m.program.interfaces.at(name);w.str(name);vector<string> methods;for(auto& kv:c->methods)methods.push_back(kv.first);sort(methods.begin(),methods.end());w.u32(static_cast<uint32_t>(methods.size()));for(auto& method:methods)writeSignature(w,c->methods.at(method));}
    w.u32(static_cast<uint32_t>(m.program.classOrder.size()));
    for (const auto& n : m.program.classOrder) {
        auto c=m.program.classes.at(n); const auto& cc=m.classes.at(n); w.str(c->name); w.str(c->base);w.u32(static_cast<uint32_t>(c->interfaces.size()));for(auto& name:c->interfaces)w.str(name);
        w.u32(static_cast<uint32_t>(c->fields.size()));
        for (size_t k=0;k<c->fields.size();++k) { const auto& f=c->fields[k]; w.str(f.name); w.str(f.type); w.u8(f.priv); w.u32(cc.fields.at(k)); }
        w.u8(c->ctor!=nullptr); if (c->ctor) { writeSignature(w,c->ctor); w.u32(cc.ctor); }
        w.u32(static_cast<uint32_t>(cc.baseArgs.size())); for (auto i : cc.baseArgs) w.u32(i);
        vector<string> methods; for (const auto& pair : c->methods) methods.push_back(pair.first); sort(methods.begin(),methods.end());
        w.u32(static_cast<uint32_t>(methods.size())); for (const auto& name : methods) { auto f=c->methods.at(name); writeSignature(w,f); w.u32(m.functions.at(functionKey(f))); }
    }
    if (w.bytes.size()>64u*1024u*1024u) throw invalid_argument("Bytecode excede o limite de 64 MiB.");
    uint32_t crc=crc32(w.bytes); w.u32(crc); return move(w.bytes);
}
void verify(const Module& m) {
    auto bad=[] { throw invalid_argument("Estrutura de bytecode invalida."); };
    auto codeId=[&](uint32_t i) { if (i>=m.codes.size()) bad(); };
    codeId(m.entry);
    for (const auto& f : m.functions) codeId(f.second);
    for (const auto& pair : m.classes) {
        const auto& cc=pair.second; if (cc.ctor!=noCode) codeId(cc.ctor);
        for (auto i : cc.fields) if (i!=noCode) codeId(i);
        for (auto i : cc.baseArgs) codeId(i);
    }
    for (const auto& name : m.program.classOrder) {
        set<string> seen; string n=name;
        while (!n.empty()) { if (!seen.insert(n).second || !m.program.classes.count(n)) bad(); n=m.program.classes.at(n)->base; }
    }
    for (const auto& c : m.codes) {
        if (c.empty()) bad();
        for (const auto& i : c) {
            if (static_cast<uint8_t>(i.op)>static_cast<uint8_t>(Op::Nop) || i.line<1 || i.col<1) bad();
            if ((i.op==Op::Jump || i.op==Op::JumpFalse || i.op==Op::JumpTrue) && (i.a<0 || static_cast<uint64_t>(i.a)>=c.size())) bad();
            if ((i.op==Op::Call || i.op==Op::Array || i.op==Op::New || i.op==Op::Unwind) && (i.a<0 || i.a>1000000)) bad();
            if (i.op==Op::Function && !builtins.count(i.x) && !m.program.functions.count(i.x)) bad();
            if (i.op==Op::New && !m.program.classes.count(i.x)) bad();
        }
        // Verifica a altura da pilha e a profundidade de escopos em todo o CFG.
        using State=pair<int,int>; map<size_t,State> states; deque<size_t> queue;
        states[0]={0,0}; queue.push_back(0);
        auto put=[&](size_t pc, State s) {
            if (pc>=c.size()) { bad(); return; }
            auto it=states.find(pc); if (it!=states.end()) { if (it->second!=s) bad(); }
            else { states[pc]=s; queue.push_back(pc); }
        };
        while (!queue.empty()) {
            size_t pc=queue.front(); queue.pop_front(); State s=states.at(pc); const auto& i=c[pc];
            int need=0, delta=0;
            switch (i.op) {
            case Op::Void: case Op::Int: case Op::Real: case Op::Text: case Op::Character: case Op::Bool: case Op::Null:
            case Op::LocalLoad:case Op::LocalRef:case Op::Default: case Op::Load: case Op::RefVar: case Op::Self: case Op::Function: delta=1; break;
            case Op::MoveVar:case Op::Member: case Op::RefField: case Op::Unary: case Op::Mutate: need=1; break;
            case Op::AddInt:case Op::SubInt:case Op::MulInt:case Op::LtInt:case Op::LeInt:case Op::GtInt:case Op::GeInt:case Op::EqInt:case Op::NeInt:case Op::Append:case Op::Index: case Op::RefIndex: case Op::StoreRef: case Op::Binary: need=2; delta=-1; break;
            case Op::ReadRef: case Op::Dup: need=1; delta=1; break;
            case Op::Closure:case Op::Array: case Op::New: need=static_cast<int>(i.a); delta=1-need; break;
            case Op::Call: need=static_cast<int>(i.a)+1; delta=-static_cast<int>(i.a); break;
            case Op::LocalDeclare:case Op::Drop: case Op::Declare: case Op::JumpFalse: case Op::JumpTrue: need=1; delta=-1; break;
            case Op::Throw:need=1;delta=-1;break;
            case Op::Try:case Op::SignalBreak:case Op::SignalContinue:break;
            case Op::Return: need=1; if (s.first!=1) bad(); break;
            case Op::Enter: ++s.second; break;
            case Op::Leave: if (--s.second<0) bad(); break;
            case Op::Unwind: s.second-=static_cast<int>(i.a); if (s.second<0) bad(); break;
            case Op::Jump: case Op::Nop: break;
            }
            if (s.first<need) bad();
            s.first+=delta;
            if (i.op==Op::Return || i.op==Op::Throw || i.op==Op::SignalBreak || i.op==Op::SignalContinue) continue;
            if (i.op==Op::Jump) put(static_cast<size_t>(i.a),s);
            else { if (i.op==Op::JumpFalse || i.op==Op::JumpTrue) put(static_cast<size_t>(i.a),s); put(pc+1,s); }
        }
    }
}
Module deserialize(const string& bytes) {
    if (bytes.size()<16 || bytes.size()>64u*1024u*1024u+4 || bytes.substr(0,8)!="UMABC03\n") throw invalid_argument("O arquivo nao e bytecode UmaScript 2.");
    Reader tail(bytes); tail.pos=bytes.size()-4; uint32_t expected=tail.u32();
    if (crc32(bytes.substr(0,bytes.size()-4))!=expected) throw invalid_argument("Bytecode corrompido: checksum invalido.");
    Reader r(bytes); r.pos=8; if (r.u32()!=3) r.bad(); Module m; m.sourceName=r.str(); m.entry=r.u32();
    uint32_t n=r.count(100000); m.codes.reserve(n);
    for (uint32_t k=0;k<n;++k) {
        uint32_t count=r.count(); Code c; c.reserve(count);
        for (uint32_t j=0;j<count;++j) {
            Instruction i; i.op=static_cast<Op>(r.u8()); uint64_t bits=r.u64(); memcpy(&i.a,&bits,8);
            i.x=r.str(); i.y=r.str(); i.line=static_cast<int>(r.u32()); i.col=static_cast<int>(r.u32()); c.push_back(move(i));
        }
        m.codes.push_back(move(c));
    }
    n=r.count(100000);
    for (uint32_t k=0;k<n;++k) { auto f=readSignature(r); auto id=r.u32(); if (f->owner!="" || !m.program.functions.emplace(f->name,f).second) r.bad(); m.program.functionOrder.push_back(f->name); m.functions[functionKey(f)]=id; }
    n=r.count(100000);
    for(uint32_t k=0;k<n;++k){auto c=make_shared<Class>();c->name=r.str();auto count=r.count(100000);for(uint32_t j=0;j<count;++j){auto f=readSignature(r);if(!c->methods.emplace(f->name,f).second)r.bad();}if(!m.program.interfaces.emplace(c->name,c).second)r.bad();}
    n=r.count(100000);
    for (uint32_t k=0;k<n;++k) {
        auto c=make_shared<Class>(); c->name=r.str(); c->base=r.str();auto interfaces=r.count(1000);for(uint32_t j=0;j<interfaces;++j)c->interfaces.push_back(r.str()); c->token={c->name,"id",1,1}; ClassCode cc;
        uint32_t count=r.count(100000);
        set<string> fields;
        for (uint32_t j=0;j<count;++j) { string name=r.str(), ty=r.str(); bool priv=r.u8()!=0; uint32_t id=r.u32(); if (!fields.insert(name).second) r.bad(); c->fields.push_back({move(ty),move(name),priv,{},c->token}); cc.fields.push_back(id); }
        if (r.u8()) { c->ctor=readSignature(r); cc.ctor=r.u32(); }
        count=r.count(4096); for (uint32_t j=0;j<count;++j) cc.baseArgs.push_back(r.u32());
        count=r.count(100000);
        for (uint32_t j=0;j<count;++j) { auto f=readSignature(r); auto id=r.u32(); if (f->owner!=c->name || !c->methods.emplace(f->name,f).second) r.bad(); m.functions[functionKey(f)]=id; }
        if (!m.program.classes.emplace(c->name,c).second) r.bad();
        m.program.classOrder.push_back(c->name); m.classes[c->name]=move(cc);
    }
    if (r.pos!=bytes.size()-4) r.bad();
    verify(m); return m;
}

struct VM : Runtime {
    Module& module;
    using Item=variant<Value,LValue>;
    explicit VM(Module& m) : Runtime(m.program), module(m) {}
    [[noreturn]] void invalid(const Token& t) { fail(t,"bytecode","Operando ou instrucao incompativel."); }
    string valueType(const Value& v) {
        switch (v.tag) {
        case Value::Void: return "power"; case Value::Int: return "speed"; case Value::Double: return "stamina";
        case Value::String: return "wit"; case Value::Char: return "guts"; case Value::Bool: return "track";
        case Value::Null: return "nature"; case Value::Obj: return v.object->cls; case Value::Arr: return v.array->elem+"[]";
        case Value::Callable:{auto b=v.bound;if(!b->function)return "@native";vector<string> args;for(size_t i=b->captures.size();i<b->function->params.size();++i)args.push_back(b->function->params[i].type);return signature(b->function->result,args);}
        }
        return "";
    }
    Value typed(Value v, const string& t, const Token& at) { if (!accepts(p,t,valueType(v))) invalid(at); return coerce(move(v),t); }
    Value binary(const Value& a,const Value& b,const Token& t,const string& op) {
        if(op=="+%" || op=="-%" || op=="*%"){if(a.tag!=Value::Int || b.tag!=Value::Int)invalid(t);return operation(a,b,t,op);}
        bool num=numeric(valueType(a)) && numeric(valueType(b));
        if(op=="&" || op=="|" || op=="^" || op=="<<" || op==">>") {if(a.tag!=Value::Int || b.tag!=Value::Int)invalid(t);}
        else if(op=="==" || op=="!=") {
            if(a.tag==Value::Void || b.tag==Value::Void || a.tag==Value::Callable || b.tag==Value::Callable)invalid(t);
        } else if(op=="<" || op==">" || op=="<=" || op==">=") {
            if(!num && !((a.tag==Value::String && b.tag==Value::String) || (a.tag==Value::Char && b.tag==Value::Char)))invalid(t);
        } else if(op=="+" && ((a.tag==Value::String && (b.tag==Value::String || b.tag==Value::Char)) || (a.tag==Value::Char && b.tag==Value::String))) {}
        else if(!num || (op=="%" && (a.tag!=Value::Int || b.tag!=Value::Int)) || (op!="+" && op!="-" && op!="*" && op!="/" && op!="%"))invalid(t);
        return operation(a,b,t,op);
    }
    Value callValue(const Value& callee,vector<Value> args,const Token& t){if(callee.tag!=Value::Callable)invalid(t);auto b=callee.bound;if(!b->native.empty())return nativeChecked(b->native,move(args),t);args.insert(args.begin(),b->captures.begin(),b->captures.end());return invoke(b->function,b->self,args,t);}
    Value nativeChecked(const string& n,vector<Value> vs,const Token& t) {
        auto count=[&](size_t low,size_t high){if(vs.size()<low || vs.size()>high)invalid(t);};
        if(n=="ordenar" && vs.size()==2){if(vs[0].tag!=Value::Arr || vs[1].tag!=Value::Callable)invalid(t);auto& a=vs[0].array->values;auto compare=vs[1];auto sorted=a;std::stable_sort(sorted.begin(),sorted.end(),[&](const Value& x,const Value& y){auto v=callValue(compare,{x,y},t);if(v.tag!=Value::Bool)invalid(t);return v.b;});a=move(sorted);return {};}
        auto stdNative=standardNatives.find(n);
        if(stdNative!=standardNatives.end()){const auto& sig=stdNative->second;count(sig.args.size(),sig.args.size());for(size_t i=0;i<vs.size();++i)vs[i]=typed(vs[i],sig.args[i],t);}
        else if(n=="bakushin"){for(const auto& v:vs)if(v.tag==Value::Void || v.tag==Value::Callable)invalid(t);}
        else if(n=="suzuka")count(0,0);
        else if(n=="flash"){count(1,2);if(vs[0].tag!=Value::Bool || (vs.size()==2 && vs[1].tag!=Value::String))invalid(t);}
        else {
            count(n=="falcon"?2:1,n=="falcon"?2:1);const auto& v=vs[0];
            if(n=="oguri"){if(v.tag!=Value::String && v.tag!=Value::Arr && v.tag!=Value::Null)invalid(t);}
            else if(n=="falcon" || n=="dober") {if(v.tag!=Value::Arr && v.tag!=Value::Null)invalid(t);if(n=="falcon" && v.tag==Value::Arr)vs[1]=typed(vs[1],v.array->elem,t);}
            else if(n=="taiki" || n=="creek"){if(!numeric(valueType(v)) && v.tag!=Value::String)invalid(t);}
            else if(n=="ordenar"){if(v.tag!=Value::Arr)invalid(t);}
            else if(n=="hayahide"){if(v.tag!=Value::Char || boundaries(v.s,t).size()!=2)invalid(t);}
            else if(n=="ticket"){if(v.tag!=Value::Int)invalid(t);}
            else if(n=="maruzensky"){if(v.tag==Value::Void || v.tag==Value::Callable)invalid(t);}
            else invalid(t);
        }
        return native(n,move(vs),t);
    }
    LValue variable(const string& n, const Token& at) {
        for (auto scope=env;scope;scope=scope->parent) if (scope->vars.count(n)) {
            string ty=scope->vars.at(n).type;
            return {ty,[scope,n]{return scope->vars.at(n).value;},[scope,n](Value v){scope->vars.at(n).value=move(v);}};
        }
        fail(at,"execucao","Variavel global ainda nao inicializada: "+n);
    }
    LValue field(const Value& v, const string& n, const Token& at) {
        if (v.tag==Value::Null) fail(at,"execucao","Acesso a nature.");
        if (v.tag!=Value::Obj) invalid(at);
        auto o=v.object;
        if (!o->fields.count(n)) fail(at,"execucao","Campo ainda nao inicializado: "+n);
        return {o->fields.at(n).type,[o,n]{return o->fields.at(n).value;},[o,n](Value x){o->fields.at(n).value=move(x);}};
    }
    LValue element(const Value& v, const Value& ix, const Token& at) {
        if (v.tag==Value::Null) fail(at,"execucao","Indice em nature.");
        if (v.tag!=Value::Arr || ix.tag!=Value::Int) invalid(at);
        auto a=v.array; size_t i=index(ix,a->values.size(),at);
        return {a->elem,[a,i,at]{if(i>=a->values.size()) fail(at,"execucao","Array mudou durante a expressao.");return a->values[i];},
            [a,i,at](Value x){if(i>=a->values.size()) fail(at,"execucao","Array mudou durante a expressao.");a->values[i]=move(x);}};
    }
    vector<Value> args(vector<Item>& stack, int64_t n, const Token& at) {
        if (n<0 || static_cast<uint64_t>(n)>stack.size()) invalid(at);
        vector<Value> vs(static_cast<size_t>(n));
        for (size_t i=vs.size();i>0;--i) { if (!holds_alternative<Value>(stack.back())) invalid(at); vs[i-1]=move(get<Value>(stack.back())); stack.pop_back(); }
        return vs;
    }
    void params(const F& f, const vector<Value>& vs, const Token& at) {
        if (f->params.size()!=vs.size()) invalid(at);
        for (size_t i=0;i<vs.size();++i) env->vars.emplace(f->params[i].name,Slot{f->params[i].type,typed(vs[i],f->params[i].type,at)});
    }
    Value invoke(const F& f, const shared_ptr<Object>& receiver, const vector<Value>& vs, const Token& at) {
        if (!f || ++depth>maxDepth) { if (f) --depth; fail(at,"execucao","Limite de chamadas atingido (--max-depth)."); }
        struct Pop { size_t& d; ~Pop(){--d;} } pop{depth};
        Context saved(*this); env=make_shared<Environment>(global); self=receiver; owner=f->owner; params(f,vs,at);
        try { try{return typed(execute(module.functions.at(functionKey(f))),f->result,at);}catch(Returned& r){return typed(move(r.value),f->result,at);} }
        catch (Error& e) { if (e.stack.size()<24) e.stack.push_back(functionKey(f)+" (linha "+to_string(at.line)+")"); throw; }
    }
    void initialize(const shared_ptr<Object>& o, const string& name, const vector<Value>& vs, const Token& at) {
        if (++depth>maxDepth) {--depth;fail(at,"execucao","Limite de construtores atingido (--max-depth).");}
        struct Pop {size_t& d;~Pop(){--d;}} pop{depth};
        const auto& cc=module.classes.at(name); auto c=p.classes.at(name);
        Context saved(*this); env=make_shared<Environment>(global); self=o; owner=name;
        if (c->ctor) params(c->ctor,vs,at); else if (!vs.empty()) invalid(at);
        try {
            if (!c->base.empty()) {vector<Value> base; for(auto i:cc.baseArgs) base.push_back(execute(i)); initialize(o,c->base,base,at);}
            auto ctorEnv=env; env=make_shared<Environment>(global);
            for(const auto& f:c->fields) o->fields.emplace(f.name,Slot{f.type,defaultValue(f.type)});
            for(size_t i=0;i<c->fields.size();++i) if(cc.fields.at(i)!=noCode) {const auto& f=c->fields[i];o->fields.at(f.name).value=typed(execute(cc.fields[i]),f.type,at);}
            env=ctorEnv;if(cc.ctor!=noCode){try{execute(cc.ctor);}catch(const Returned&){}}
        } catch(Error& e) {if(e.stack.size()<24)e.stack.push_back(name+".opera (linha "+to_string(at.line)+")");throw;}
    }
    struct LocalFrame {deque<Slot> slots;};
    Value execute(uint32_t id,shared_ptr<LocalFrame> frame={},size_t startPc=0) {
        if(!frame)frame=make_shared<LocalFrame>();
        vector<vector<size_t>> active(1);
        auto leave=[&](){if(active.size()<2)invalid({});for(auto slot:active.back())frame->slots.at(slot).value={};active.pop_back();};
        auto local=[&](int64_t slot,const Token& t)->Slot&{if(slot<0 || static_cast<uint64_t>(slot)>=frame->slots.size() || frame->slots[static_cast<size_t>(slot)].type.empty())invalid(t);return frame->slots[static_cast<size_t>(slot)];};

        const Code& code=module.codes.at(id); vector<Item> stack; stack.reserve(32); auto savedEnv=env;
        struct Restore {shared_ptr<Environment>& target;shared_ptr<Environment> saved;~Restore(){target=move(saved);}} restore{env,savedEnv};
        vector<Slot*> globalSlots(code.size(),nullptr);
        auto globalSlot=[&](size_t instruction,const string& name)->Slot*{if(env!=global && env->vars.count(name))return nullptr;auto& slot=globalSlots[instruction];if(!slot){auto it=global->vars.find(name);if(it!=global->vars.end())slot=&it->second;}return slot;};
        size_t pc=startPc;
        while(pc<code.size()) {
            const auto& i=code[pc++]; Token t=i.token(); tick(t);
            auto popValue=[&]() {if(stack.empty() || !holds_alternative<Value>(stack.back()))invalid(t);Value v=move(get<Value>(stack.back()));stack.pop_back();return v;};
            auto popRef=[&]() {if(stack.empty() || !holds_alternative<LValue>(stack.back()))invalid(t);LValue v=move(get<LValue>(stack.back()));stack.pop_back();return v;};
            auto push=[&](Value v){stack.emplace_back(move(v));};
            switch(i.op) {
            case Op::Try:{
                auto colon=i.x.find(':');auto handler=i.x.substr(0,colon),cleanup=i.x.substr(colon+1);auto bar=i.y.find('|');auto name=i.y.substr(0,bar);
                auto before=env;std::exception_ptr pending;
                try{execute(static_cast<uint32_t>(i.a),frame);}catch(const Error& e){
                    if(e.phase!="execucao" && e.phase!="texto")pending=std::current_exception();
                    else if(handler!="-"){env=make_shared<Environment>(before);env->vars.emplace(name,Slot{"wit",Value::text(e.what())});try{execute(static_cast<uint32_t>(stoul(handler)),frame);}catch(...){pending=std::current_exception();}}
                    else pending=std::current_exception();
                }catch(...){pending=std::current_exception();}
                env=before;if(cleanup!="-")execute(static_cast<uint32_t>(stoul(cleanup)),frame);
                if(pending){try{std::rethrow_exception(pending);}catch(const Broken&){if(bar==string::npos)throw;auto e=i.y.find("|end=");auto sep=i.y.find("|",bar+1);auto target=stoull(i.y.substr(sep+1));while(active.size()>target+1)leave();pc=stoull(i.y.substr(e+5));}catch(const Continued&){if(bar==string::npos)throw;auto sep=i.y.find("|",bar+1);auto target=stoull(i.y.substr(sep+1));while(active.size()>target+1)leave();pc=stoull(i.y.substr(bar+1));}}
                break;
            }
            case Op::Throw:{auto v=popValue();if(v.tag!=Value::String)invalid(t);fail(t,"execucao",v.s.str());}
            case Op::SignalBreak:throw Broken{};
            case Op::SignalContinue:throw Continued{};
            case Op::MoveVar:{auto r=popRef();auto value=r.read();r.write(Value::null());push(move(value));break;}
            case Op::Append:{auto v=popValue();auto r=popRef();auto old=r.read();if(old.tag!=Value::String || (v.tag!=Value::String && v.tag!=Value::Char))invalid(t);r.write({});old.s.append(v.s);r.write(old);push(move(old));break;}
            case Op::Void: push({});break;
            case Op::Int: push(Value::integer(i.a));break;
            case Op::Real: {double d;memcpy(&d,&i.a,8);push(finite(d,t));break;}
            case Op::Text: case Op::Character: push(Value::text(i.x,i.op==Op::Character));break;
            case Op::Bool: push(Value::boolean(i.a!=0));break;
            case Op::Null: push(Value::null());break;
            case Op::Default: push(defaultValue(i.x));break;
            case Op::LocalLoad:push(local(i.a,t).value);break;
            case Op::LocalRef:{auto slot=&local(i.a,t);stack.emplace_back(LValue{slot->type,[slot]{return slot->value;},[slot](Value v){slot->value=move(v);}});break;}
            case Op::LocalDeclare:{auto v=typed(popValue(),i.y,t);if(i.a<0 || i.a>1000000)invalid(t);while(frame->slots.size()<=static_cast<size_t>(i.a))frame->slots.push_back({});frame->slots[static_cast<size_t>(i.a)]={i.y,move(v)};active.back().push_back(static_cast<size_t>(i.a));break;}
            case Op::Load:if(auto slot=globalSlot(pc-1,i.x))push(slot->value);else push(variable(i.x,t).read());break;
            case Op::RefVar:if(auto slot=globalSlot(pc-1,i.x))stack.emplace_back(LValue{slot->type,[slot]{return slot->value;},[slot](Value v){slot->value=move(v);}});else stack.emplace_back(variable(i.x,t));break;
            case Op::Self: if(!self)invalid(t);push(Value::instance(self));break;
            case Op::Closure:{auto captures=args(stack,i.a,t);auto v=Value::function(p.functions.at(i.x));v.bound->captures=move(captures);push(move(v));break;}
            case Op::Function:
                if(builtins.count(i.x))push(Value::function({},{},i.x));else push(Value::function(p.functions.at(i.x)));
                break;
            case Op::Member: {
                Value v=popValue();if(v.tag==Value::Null)fail(t,"execucao","Acesso a nature.");if(v.tag!=Value::Obj)invalid(t);
                string start=v.object->cls;if(i.a){if(owner.empty())invalid(t);start=p.classes.at(owner)->base;}
                auto m=member(p,start,i.x);if(m.owner.empty())invalid(t);
                if(m.method)push(Value::function(m.method,v.object));else push(field(v,i.x,t).read());break;
            }
            case Op::RefField: {Value v=popValue();stack.emplace_back(field(v,i.x,t));break;}
            case Op::Index: {
                Value ix=popValue(),v=popValue();if(v.tag==Value::Null)fail(t,"execucao","Indice em nature.");if(ix.tag!=Value::Int)invalid(t);
                if(v.tag==Value::Arr)push(v.array->values[index(ix,v.array->values.size(),t)]);
                else {if(v.tag!=Value::String)invalid(t);try{push(Value::text(v.s.at(ix.i,{t.file.c_str(),t.line,t.col}),true));}catch(const uma_native::Failure& e){fail(t,"execucao",e.what());}}break;
            }
            case Op::RefIndex: {Value ix=popValue(),v=popValue();stack.emplace_back(element(v,ix,t));break;}
            case Op::ReadRef: if(stack.empty() || !holds_alternative<LValue>(stack.back()))invalid(t);push(get<LValue>(stack.back()).read());break;
            case Op::StoreRef: {Value v=popValue();auto l=popRef();v=typed(move(v),l.type,t);l.write(v);push(move(v));break;}
            case Op::Mutate: {auto l=popRef();Value old=l.read();if(!numeric(valueType(old)))invalid(t);Value v=typed(operation(old,Value::integer(1),t,i.x=="++"?"+":"-"),l.type,t);l.write(v);push(i.a?old:v);break;}
            case Op::Array: {auto vs=args(stack,i.a,t);auto a=array(i.x);for(auto& v:vs)a->values.push_back(typed(move(v),i.x,t));push(Value::list(a));break;}
            case Op::New: {auto vs=args(stack,i.a,t);allocation();auto o=make_shared<Object>();o->cls=i.x;objects.push_back(o);initialize(o,i.x,vs,t);push(Value::instance(o));break;}
            case Op::Unary: {
                Value v=popValue();if(i.x=="!"){if(v.tag!=Value::Bool)invalid(t);push(Value::boolean(!v.b));}
                else if(i.x=="~"){if(v.tag!=Value::Int)invalid(t);push(Value::integer(~v.i));}
                else {if(!numeric(valueType(v)))invalid(t);if(i.x=="+")push(v);else if(v.tag==Value::Double)push(Value::real(-v.d));else push(operation(Value::integer(0),v,t,"-"));}break;
            }
            case Op::AddInt:case Op::SubInt:case Op::MulInt:case Op::LtInt:case Op::LeInt:case Op::GtInt:case Op::GeInt:case Op::EqInt:case Op::NeInt:{auto b=popValue(),a=popValue();if(a.tag!=Value::Int || b.tag!=Value::Int)invalid(t);try{uma_native::Loc l{t.file.c_str(),t.line,t.col};switch(i.op){case Op::AddInt:push(Value::integer(uma_native::add(a.i,b.i,l)));break;case Op::SubInt:push(Value::integer(uma_native::sub(a.i,b.i,l)));break;case Op::MulInt:push(Value::integer(uma_native::mul(a.i,b.i,l)));break;case Op::LtInt:push(Value::boolean(a.i<b.i));break;case Op::LeInt:push(Value::boolean(a.i<=b.i));break;case Op::GtInt:push(Value::boolean(a.i>b.i));break;case Op::GeInt:push(Value::boolean(a.i>=b.i));break;case Op::EqInt:push(Value::boolean(a.i==b.i));break;default:push(Value::boolean(a.i!=b.i));}}catch(const uma_native::Failure& e){fail(t,"execucao",e.what());}break;}
            case Op::Binary: {Value b=popValue(),a=popValue();push(binary(a,b,t,i.x));break;}
            case Op::Call: {
                auto vs=args(stack,i.a,t);Value callee=popValue();if(callee.tag!=Value::Callable || !callee.bound)invalid(t);
                push(callValue(callee,move(vs),t));
                break;
            }
            case Op::Dup: if(stack.empty())invalid(t);stack.push_back(stack.back());break;
            case Op::Drop: popValue();break;
            case Op::Declare: {Value v=typed(popValue(),i.y,t);if(!env->vars.emplace(i.x,Slot{i.y,move(v)}).second)invalid(t);break;}
            case Op::Enter:active.emplace_back();break;
            case Op::Leave:leave();break;
            case Op::Unwind:for(int64_t k=0;k<i.a;++k)leave();break;
            case Op::Jump: pc=static_cast<size_t>(i.a);break;
            case Op::JumpFalse: case Op::JumpTrue: {Value v=popValue();if(v.tag!=Value::Bool)invalid(t);if(v.b==(i.op==Op::JumpTrue))pc=static_cast<size_t>(i.a);break;}
            case Op::Return: {auto v=popValue();if(i.a)throw Returned{move(v)};return v;}
            case Op::Nop: break;
            }
        }
        invalid({});
    }
    void run(){try{execute(module.entry);}catch(const Returned&){invalid({});}catch(const Broken&){invalid({});}catch(const Continued&){invalid({});}}
};

void disassemble(const Module& m, ostream& out) {
    out<<"UmaScript bytecode v2 | "<<m.sourceName<<" | entry="<<m.entry<<'\n';
    for(size_t k=0;k<m.codes.size();++k){out<<"\nchunk "<<k<<":\n";size_t pc=0;for(const auto& i:m.codes[k]){
        out<<setw(5)<<pc++<<"  "<<setw(12)<<left<<opName(i.op)<<right<<" "<<i.a;
        if(!i.x.empty())out<<" "<<quoted(i.x);
        if(!i.y.empty())out<<" "<<quoted(i.y);
        out<<"  ; "<<i.line<<":"<<i.col<<'\n';
    }}
}
} // namespace uma
