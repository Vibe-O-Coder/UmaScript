#pragma once
#include "language.hpp"

namespace uma {
// Lowering tipado: numeros locais viram int64_t/double na pilha, sem Value/VM.
struct NativeEmitter {
    Program& p; string path, owner; int next=0;set<string> recursive;
    bool plainFields(const C& c){if(!c->base.empty())return false;for(auto& kv:p.classes)if(kv.second->base==c->name)return false;return true;}
    struct Symbol {string name,type;bool global;};
    vector<map<string,Symbol>> scopes{1};
    map<string,string> classes,functions,members;
    explicit NativeEmitter(Program& p,string path):p(p),path(move(path)) {
        map<string,set<string>> calls;set<string> dynamic;
        for(auto& n:p.functionOrder){auto f=p.functions.at(n);function<void(const E&)> ex;function<void(const S&)> st;
            ex=[&](const E& e){if(!e)return;if(e->namedFunction)calls[n].insert(e->name);if(e->kind==Expr::Lambda)calls[n].insert(e->name);if(e->kind==Expr::Call && !e->a->namedFunction && !builtins.count(e->a->name))dynamic.insert(n);ex(e->a);ex(e->b);for(auto& a:e->args)ex(a);};
            st=[&](const S& s){if(!s)return;ex(s->expr);ex(s->cond);ex(s->update);st(s->init);st(s->yes);st(s->no);st(s->cleanup);for(auto& c:s->body)st(c);};st(f->body);
        }
        for(auto& n:p.functionOrder){set<string> seen;function<bool(string)> visit=[&](string current){if(current==n && !seen.empty())return true;if(!seen.insert(current).second)return false;for(auto& to:calls[current])if(visit(to))return true;return false;};if(dynamic.count(n) || visit(n))recursive.insert(n);}
        for(auto& kv:p.interfaces){classes[kv.first]="UI"+to_string(next++);for(auto& f:kv.second->methods)memberName(f.first);}
        for(const auto& n:p.classOrder)classes[n]="UC"+to_string(next++);
        for(const auto& n:p.functionOrder)functions[n]="UF"+to_string(next++);
        for(const auto& n:p.classOrder){auto c=p.classes.at(n);for(const auto& f:c->fields)memberName(f.name);for(const auto& f:c->methods)memberName(f.first);}
        for(const auto& s:p.statements)if(s->kind==Stmt::Variable)scopes[0][s->name]={"UG"+to_string(next++),s->type,true};
    }
    string memberName(const string& n){if(!members.count(n))members[n]="um"+to_string(next++);return members.at(n);}
    static string quote(const string& s){string out="\"";for(unsigned char c:s){if(c=='\\'||c=='\"'){out+='\\';out+=char(c);}else if(c>=32 && c<127)out+=char(c);else {out+='\\';out+=char('0'+(c>>6));out+=char('0'+((c>>3)&7));out+=char('0'+(c&7));}}return out+'\"';}
    string sourcePath(const Token& t){auto f=filesystem::u8path(t.file.empty()?path:t.file);auto base=filesystem::u8path(path).parent_path();auto relative=f.lexically_relative(base);if(relative.empty() || relative.u8string().rfind("..",0)==0)return f.filename().u8string();return relative.generic_u8string();}
    string loc(const Token& t){return "Loc{"+quote(sourcePath(t))+","+to_string(t.line)+","+to_string(t.col)+"}";}
    string type(const string& t){if(functionType(t)){auto parts=functionParts(t);string out="Fn<"+type(parts[0])+"(";for(size_t i=1;i<parts.size();++i){if(i>1)out+=",";out+=type(parts[i]);}return out+")>";}if(arrayType(t))return "A<"+type(elementType(t))+">";if(classes.count(t))return "std::shared_ptr<"+classes.at(t)+">";if(t=="speed")return "I";if(t=="stamina")return "double";if(t=="wit")return "Text";if(t=="guts")return "char32_t";if(t=="track")return "bool";if(t=="nature")return "std::nullptr_t";return "void";}
    string def(const string& t){return arrayType(t)?"array<"+type(elementType(t))+">()":t=="nature"?"nullptr":type(t)+"{}";}
    Symbol symbol(const string& n){for(auto it=scopes.rbegin();it!=scopes.rend();++it)if(it->count(n))return it->at(n);throw logic_error("Simbolo ausente no backend: "+n);}
    string variable(const E& e){auto s=symbol(e->name);return s.name+(s.global?".get("+loc(e->token)+")":"");}
    string self(){return "std::static_pointer_cast<"+classes.at(owner)+">(shared_from_this())";}
    string ref(const E& e){
        if(e->kind==Expr::Var)return "uma_native::ref("+variable(e)+")";
        if(e->kind==Expr::Get){auto m=member(p,e->a->type,e->name);string c=classes.at(m.owner);return "field(std::static_pointer_cast<"+c+">("+expr(e->a)+"),&"+c+"::"+memberName(e->name)+","+loc(e->token)+")";}
        return "([&](){auto a="+expr(e->a)+";auto i="+expr(e->b)+";return element(a,i,"+loc(e->token)+");}())";
    }
    string operation(string a,string b,string ta,string tb,string op,const Token& t){
        string l=loc(t);
        if(op=="+%" || op=="-%" || op=="*%")return string(op=="+%"?"wrapAdd":op=="-%"?"wrapSub":"wrapMul")+"("+a+","+b+")";
        if(op=="==" || op=="!=")return (op=="!="?"!":"")+string("equal(")+a+","+b+")";
        if(op=="<" || op==">" || op=="<=" || op==">=")return "(compare("+a+","+b+")"+op+"0)";
        if(op=="&" || op=="|" || op=="^" || op=="<<" || op==">>")return "bitop("+a+","+b+","+quote(op)+","+l+")";
        if(op=="+" && (ta=="wit" || tb=="wit"))return "concat("+a+","+b+")";
        bool real=ta=="stamina" || tb=="stamina";
        if(real){a="static_cast<double>("+a+")";b="static_cast<double>("+b+")";if(op=="/")return "divide("+a+","+b+","+l+")";return "finite("+a+op+b+","+l+")";}
        string f=op=="+"?"add":op=="-"?"sub":op=="*"?"mul":op=="/"?"divide":"modulo";
        return f+"("+a+","+b+","+l+")";
    }
    string native(const string& n,const vector<string>& a,const Token& t){
        string l=loc(t),args;for(const auto& s:a){if(!args.empty())args+=",";args+=s;}

        if(n=="uma_json_normalize")return "jsonNormalize("+args+","+l+")";
        if(n=="uma_json_get")return "jsonGet("+args+","+l+")";
        if(n=="uma_json_at")return "jsonAt("+args+","+l+")";
        if(n=="uma_json_kind")return "jsonKind("+args+","+l+")";
        if(n=="uma_json_value")return "jsonValue("+args+","+l+")";
        if(n=="uma_json_size")return "jsonSize("+args+","+l+")";
        if(n=="uma_json_set")return "jsonSet("+args+","+l+")";
        if(n=="uma_json_quote")return "jsonQuote("+args+")";
        if(n=="uma_base64_encode")return "encodeBytes("+args+","+l+")";
        if(n=="uma_base64_decode")return "decodeBytes("+args+","+l+")";
        if(n=="uma_args")return "argsArray()";
        if(n=="uma_eof")return "inputEnd()";
        if(n=="uma_read_all")return "readAll("+l+")";
        if(n=="uma_exit")return "exitProgram("+args+","+l+")";
        if(n=="uma_stderr")return "(std::cerr<<format("+args+")<<'\\n',void())";
        if(n=="uma_hash")return "hashText("+args+")";
        if(n=="uma_replace")return "replaceText("+args+","+l+")";
        if(n=="uma_lower" || n=="uma_upper")return "asciiCase("+args+","+(n=="uma_upper"?"true":"false")+")";
        if(n=="uma_regex_replace")return "regexReplace("+args+","+l+")";
        if(n=="uma_mkdir")return "mkdir("+args+","+l+")";
        if(n=="uma_remove")return "removePath("+args+","+l+")";
        if(n=="uma_list_dir")return "directoryArray("+args+","+l+")";
        if(n=="uma_path_join")return "pathJoin("+args+","+l+")";
        if(n=="uma_basename")return "std::filesystem::u8path(pathText("+args+","+l+")).filename().u8string()";
        if(n=="uma_random")return "randomInt()";
        if(n=="uma_unix_time")return "unixTime()";
        if(n=="uma_collect")return "collect()";
        if(n=="ordenar")return "sortArray("+args+","+l+")";
        if(n=="bakushin")return "print("+args+")";
        if(n=="suzuka")return "input("+l+")";
        if(n=="flash")return "assertion("+a[0]+","+l+(a.size()==2?","+a[1]:"")+")";
        if(n=="maruzensky")return "format("+args+")";
        if(n=="hayahide")return "static_cast<I>("+args+")";
        map<string,string> names={{"oguri","length"},{"falcon","push"},{"dober","pop"},{"taiki","toInt"},{"creek","toReal"},{"ticket","ticket"},{"uma_slice","slice"},{"uma_find","find"},{"uma_read_text","read"},{"uma_write_text","write"},{"uma_exists","exists"},{"uma_env","env"},{"uma_sleep","sleep"},{"uma_read_bytes","readBytes"},{"uma_write_bytes","writeBytes"}};
        if(names.count(n))return names.at(n)+"("+args+(args.empty()?"":",")+l+")";
        if(n=="uma_append_text")return "write("+args+","+l+",true)";
        if(n=="uma_clock")return "uma_native::clock()";
        return "finite(std::"+n.substr(4)+"("+args+"),"+l+")";
    }
    string expr(const E& e){
        string l=loc(e->token),op=e->token.text;
        switch(e->kind){
        case Expr::Literal:
            if(e->type=="wit")return "Text("+quote(e->token.text)+","+to_string(e->token.text.size())+")";
            if(e->type=="guts")return "character(std::string("+quote(e->token.text)+","+to_string(e->token.text.size())+"),"+l+")";
            if(e->type=="nature")return "nullptr";
            if(e->type=="track")return op=="turf"?"true":"false";
            return e->type=="speed"?"I{"+to_string(stoll(op))+"}":"double{"+op+"}";
        case Expr::Var:return e->namedFunction?functions.at(e->name):variable(e);
        case Expr::Lambda:{string out="([&](){";vector<string> args;for(size_t i=0;i<e->args.size();++i){auto n="capture"+to_string(i);out+="auto "+n+"="+expr(e->args[i])+";";args.push_back(n);}auto parts=functionParts(e->type);string ps;for(size_t i=1;i<parts.size();++i){if(i>1)ps+=",";auto n="lambdaArg"+to_string(i);ps+=type(parts[i])+" "+n;args.push_back(n);}string values;for(auto& a:args){if(!values.empty())values+=",";values+=a;}string captured;for(size_t i=0;i<e->args.size();++i){if(i)captured+=",";captured+="capture"+to_string(i);}return out+"return makeClosure<"+type(parts[0])+"("+[&](){string types;for(size_t i=1;i<parts.size();++i){if(i>1)types+=",";types+=type(parts[i]);}return types;}()+")>("+functions.at(e->name)+(captured.empty()?"":","+captured)+");}())";}
        case Expr::Move:return "std::move("+expr(e->a)+")";
        case Expr::Borrow:return expr(e->a);
        case Expr::Self:return self();
        case Expr::Super:return "std::static_pointer_cast<"+classes.at(p.classes.at(owner)->base)+">("+self()+")";
        case Expr::Array:{string ty=type(elementType(e->type));string s="array<"+ty+">({";for(size_t i=0;i<e->args.size();++i){if(i)s+=",";s+="static_cast<"+ty+">("+expr(e->args[i])+")";}return s+"})";}
        case Expr::Index:return e->a->type=="wit"?"([&](){auto a="+expr(e->a)+";auto i="+expr(e->b)+";return at(a,i,"+l+");}())":ref(e)+".read()";
        case Expr::Get:return ref(e)+".read()";
        case Expr::Assign:{if(op=="+=" && e->type=="wit" && (e->b->kind==Expr::Literal || e->b->kind==Expr::Var))return "([&](){auto r="+ref(e->a)+";auto v="+expr(e->b)+";return append(r,v);}())";string s="([&](){auto r="+ref(e->a)+";";if(op!="=")s+="auto old=r.read();";s+="auto v="+expr(e->b)+";return r.write(";s+=op=="="?"v":operation("old","v",e->type,e->b->type,op.substr(0,op.size()-1),e->token);return s+");}())";}
        case Expr::Postfix:case Expr::Unary:
            if(op=="++" || op=="--")return "([&](){auto r="+ref(e->a)+";auto old=r.read();auto v=r.write("+operation("old","I{1}",e->type,"speed",op=="++"?"+":"-",e->token)+");return "+(e->kind==Expr::Postfix?"old":"v")+";}())";
            if(op=="-" && e->a->kind==Expr::Literal && e->a->token.text=="9223372036854775808")return "I{INT64_MIN}";
            if(op=="!")return "(!"+expr(e->a)+")";
            if(op=="~")return "bits(~static_cast<std::uint64_t>("+expr(e->a)+"))";
            if(op=="+")return expr(e->a);
            return e->type=="speed"?"sub(I{0},"+expr(e->a)+","+l+")":"(-"+expr(e->a)+")";
        case Expr::Binary:
            if(op=="&&" || op=="||")return "("+expr(e->a)+op+expr(e->b)+")";
            return "([&](){auto a="+expr(e->a)+";auto b="+expr(e->b)+";return "+operation("a","b",e->a->type,e->b->type,op,e->token)+";}())";
        case Expr::New:case Expr::Call:{
            string s="([&]() -> "+type(e->type)+" {", callee;vector<string> args;
            if(e->kind==Expr::Call && e->a->kind==Expr::Get && member(p,e->a->a->type,e->a->name).method){s+="auto receiver="+expr(e->a->a)+";auto object=require(receiver,"+l+");";callee="object->"+(e->a->a->kind==Expr::Super?classes.at(p.classes.at(owner)->base)+"::":"")+memberName(e->a->name);}
            for(size_t i=0;i<e->args.size();++i){string n="arg"+to_string(i);s+="auto "+n+"="+expr(e->args[i])+";";args.push_back(n);}
            string callargs;for(const auto& a:args){if(!callargs.empty())callargs+=",";callargs+=a;}
            if(e->kind==Expr::New){string c=classes.at(e->name);s+="auto object=std::make_shared<"+c+">();track(object);object->init_"+c+"("+callargs+");return object;";}
            else if(e->a->kind==Expr::Var && builtins.count(e->a->name))s+="return "+native(e->a->name,args,e->token)+";";
            else s+="return "+(callee.empty()?expr(e->a):callee)+"("+callargs+");";
            return s+"}())";
        }
        }
        throw logic_error("Expressao desconhecida no backend.");
    }
    string declare(const S& s,bool global=false){string init=s->expr?expr(s->expr):def(s->type);if(global){auto n=scopes[0].at(s->name).name;return n+".value="+init+";"+n+".initialized=true;";}string n="uv"+to_string(next++);scopes.back()[s->name]={n,s->type,false};return type(s->type)+" "+n+"="+init+";";}
    string stmt(const S& s,bool global=false){return "\n#line "+to_string(s->token.line)+" "+quote(sourcePath(s->token))+"\n"+statementCode(s,global);}
    string statementCode(const S& s,bool global=false){
        string l=loc(s->token);
        switch(s->kind){
        case Stmt::Block:{if(s->mode=="group"){string out;for(auto& child:s->body)out+=stmt(child,global);return out;}scopes.emplace_back();string out="{\n";for(const auto& child:s->body)out+=stmt(child);scopes.pop_back();return out+"}\n";}
        case Stmt::Variable:return declare(s,global)+"\n";
        case Stmt::Expression:return s->expr?expr(s->expr)+";\n":";\n";
        case Stmt::If:{string cond=expr(s->cond),yes=stmt(s->yes),no=s->no?stmt(s->no):"";return "if("+cond+")"+yes+(s->no?"else "+no:"");}
        case Stmt::While:return "while("+expr(s->cond)+"){tick("+l+");"+stmt(s->yes)+"}\n";
        case Stmt::For:{scopes.emplace_back();string init=s->init?stmt(s->init):";";string cond=s->cond?expr(s->cond):"true",update=s->update?expr(s->update):"",body=stmt(s->yes);scopes.pop_back();return "{for("+init+cond+";"+update+"){tick("+l+");"+body+"}}\n";}
        case Stmt::ForEach:throw logic_error("foreach nao foi resolvido");
        case Stmt::Throw:return "error("+l+","+expr(s->expr)+");\n";
        case Stmt::Try:{
            string out="{\n",guard="cleanup"+to_string(next++);
            if(s->cleanup)out+="auto "+guard+"=finally([&]()"+stmt(s->cleanup)+");\ntry {\n";
            if(s->no){out+="try "+stmt(s->yes)+"catch(const Failure& failure){\n";scopes.emplace_back();string n="error"+to_string(next++);scopes.back()[s->name]={n,"wit",false};out+="Text "+n+"=failure.what();\n";out+=stmt(s->no);scopes.pop_back();out+="}\n";}else out+=stmt(s->yes);
            if(s->cleanup)out+="}catch(...){"+guard+".armed=false;"+guard+".action();throw;}\n";
            return out+"}\n";
        }
        case Stmt::Return:return "return"+(s->expr?" "+expr(s->expr):"")+";\n";
        case Stmt::Break:return "break;\n";
        case Stmt::Continue:return "continue;\n";
        }
        return "";
    }
    string params(const vector<Param>& ps,bool bind){string out;for(size_t i=0;i<ps.size();++i){if(i)out+=",";string n="up"+to_string(i);out+=type(ps[i].type)+" "+n;if(bind)scopes.back()[ps[i].name]={n,ps[i].type,false};}return out;}
    string generate(const string& runtime){
        string out=runtime+"\nusing namespace uma_native;\n";
        for(auto& kv:p.interfaces)out+="struct "+classes.at(kv.first)+";\n";
        for(const auto& n:p.classOrder)out+="struct "+classes.at(n)+";\n";
        for(const auto& n:p.functionOrder){auto f=p.functions.at(n);out+=type(f->result)+" "+functions.at(n)+"("+params(f->params,false)+");\n";}
        for(const auto& x:scopes[0])out+="Slot<"+type(x.second.type)+"> "+x.second.name+";\n";
        vector<string> interfaceNames;for(auto& kv:p.interfaces)interfaceNames.push_back(kv.first);sort(interfaceNames.begin(),interfaceNames.end());
        for(auto& n:interfaceNames){auto c=p.interfaces.at(n);out+="struct "+classes.at(n)+" {virtual ~"+classes.at(n)+"()=default;virtual const char* name() const=0;\n";vector<string> methods;for(auto& kv:c->methods)methods.push_back(kv.first);sort(methods.begin(),methods.end());for(auto& method:methods){auto f=c->methods.at(method);out+="virtual "+type(f->result)+" "+memberName(method)+"("+params(f->params,false)+")=0;\n";}out+="};\n";}
        set<string> done;vector<string> order;
        function<void(string)> visit=[&](string n){if(done.count(n))return;auto c=p.classes.at(n);if(!c->base.empty())visit(c->base);done.insert(n);order.push_back(n);};for(const auto& n:p.classOrder)visit(n);
        for(const auto& n:order){auto c=p.classes.at(n);string cn=classes.at(n);out+="struct "+cn+":"+(c->base.empty()?"Object":classes.at(c->base));for(auto& contract:c->interfaces)out+=","+classes.at(contract);out+=" {\nconst char* name() const override {return "+quote(n)+";}\n";for(const auto& f:c->fields)out+=(plainFields(c)?"FieldSlot<":"Slot<")+type(f.type)+"> "+memberName(f.name)+";\n";out+="void init_"+cn+"("+params(c->ctor?c->ctor->params:vector<Param>{},false)+");\n";vector<string> ms;for(const auto& m:c->methods)ms.push_back(m.first);sort(ms.begin(),ms.end());for(const auto& m:ms){auto f=c->methods.at(m);out+="virtual "+type(f->result)+" "+memberName(m)+"("+params(f->params,false)+");\n";}out+="void clear() override {";if(!c->base.empty())out+=classes.at(c->base)+"::clear();";for(const auto& f:c->fields)if(refType(p,f.type) || functionType(f.type))out+=memberName(f.name)+".value={};";out+="}\nvoid trace(const std::function<void(const std::shared_ptr<Object>&)>& visit)const override{";if(!c->base.empty())out+=classes.at(c->base)+"::trace(visit);";for(const auto& f:c->fields)if(refType(p,f.type) || functionType(f.type))out+="traceValue("+memberName(f.name)+".value,visit);";out+="}\n};\n";}
        for(const auto& n:order){auto c=p.classes.at(n);owner=n;string cn=classes.at(n);scopes.emplace_back();string ps=params(c->ctor?c->ctor->params:vector<Param>{},true);out+="void "+cn+"::init_"+cn+"("+ps+"){Frame frame("+loc(c->token)+");\n";
            if(!c->base.empty()){string args;vector<string> values;if(c->ctor)for(size_t i=0;i<c->ctor->baseArgs.size();++i){string bn="base"+to_string(i);out+="auto "+bn+"="+expr(c->ctor->baseArgs[i])+";";values.push_back(bn);}for(const auto& v:values){if(!args.empty())args+=",";args+=v;}out+="init_"+classes.at(c->base)+"("+args+");\n";}
            for(const auto& f:c->fields){string m=memberName(f.name);out+=m+".value="+def(f.type)+";"+(plainFields(c)?"":m+".initialized=true;")+"\n";}
            auto ctorScope=scopes.back();scopes.back().clear();for(const auto& f:c->fields)if(f.init)out+=memberName(f.name)+".value="+expr(f.init)+";\n";scopes.back()=ctorScope;
            if(c->ctor){for(const auto& s:c->ctor->body->body)out+=stmt(s);}out+="}\n";scopes.pop_back();
            vector<string> ms;for(const auto& m:c->methods)ms.push_back(m.first);sort(ms.begin(),ms.end());for(const auto& m:ms){auto f=c->methods.at(m);scopes.emplace_back();string signature=params(f->params,true);out+=type(f->result)+" "+cn+"::"+memberName(m)+"("+signature+"){Frame frame("+loc(f->token)+");\n";for(const auto& s:f->body->body)out+=stmt(s);out+="}\n";scopes.pop_back();}
        }
        owner.clear();for(const auto& n:p.functionOrder){auto f=p.functions.at(n);scopes.emplace_back();string ps=params(f->params,true);out+=type(f->result)+" "+functions.at(n)+"("+ps+"){"+(recursive.count(n)?"Frame frame(":"tick(")+loc(f->token)+");\n";for(const auto& s:f->body->body)out+=stmt(s);out+="}\n";scopes.pop_back();}
        out+="int main(int argc,char** argv){arguments=programArguments(argc,argv);\n#ifdef _WIN32\nSetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);\n#endif\nSession session;try{\n";for(const auto& s:p.statements)out+=stmt(s,true);out+="return 0;}catch(const Exit& e){return e.code;}catch(const Failure& e){std::cerr<<e.at.file<<':'<<e.at.line<<':'<<e.at.col<<\": erro de execucao: \"<<e.what()<<'\\n';return 1;}catch(const std::exception& e){std::cerr<<\"UmaScript: \"<<e.what()<<'\\n';return 1;}}\n";return out;
    }
};
}


