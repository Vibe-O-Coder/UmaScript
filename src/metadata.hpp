#pragma once
#include "language.hpp"
namespace uma {
string jsonString(const string& s){const char* hex="0123456789abcdef";string out="\"";for(unsigned char c:s){if(c=='\"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out+'\"';}
string signature(const F& f){string s=f->result+" "+(f->owner.empty()?"":f->owner+".")+f->name+"(";for(size_t i=0;i<f->params.size();++i){if(i)s+=", ";s+=f->params[i].type+" "+f->params[i].name;}return s+")";}
string functionKeyForMetadata(const F& f){return f->owner.empty()?f->name:f->owner+"."+f->name;}
string metadata(Program& p){
    string out="{\"diagnostics\":[],\"symbols\":[";bool first=true;
    auto symbol=[&](const string& name,const string& kind,const string& ty,const string& sig,const string& owner,const Token& t,bool priv=false){if(!first)out+=",";first=false;out+="{\"name\":"+jsonString(name)+",\"kind\":"+jsonString(kind)+",\"type\":"+jsonString(ty)+",\"signature\":"+jsonString(sig)+",\"container\":"+jsonString(owner)+",\"private\":"+(priv?"true":"false")+",\"file\":"+jsonString(t.file)+",\"line\":"+to_string(t.line)+",\"column\":"+to_string(t.col)+"}";};
    function<void(const S&,const string&)> stmt=[&](const S& s,const string& scope){if(!s)return;if(s->kind==Stmt::Variable)symbol(s->name,"variable",s->type,s->type+" "+s->name,scope,s->token);for(const auto& c:s->body)stmt(c,scope);stmt(s->init,scope);stmt(s->yes,scope);stmt(s->no,scope);stmt(s->cleanup,scope);};
    auto fn=[&](const F& f){symbol(f->name,f->owner.empty()?"function":f->name=="opera"?"constructor":"method",f->result,signature(f),f->owner,f->token,f->priv);for(const auto& a:f->params)symbol(a.name,"parameter",a.type,a.type+" "+a.name,functionKeyForMetadata(f),a.token);stmt(f->body,functionKeyForMetadata(f));};
    for(const auto& s:p.statements)stmt(s,"");
    for(const auto& n:p.functionOrder)fn(p.functions.at(n));
    for(const auto& n:p.classOrder){auto c=p.classes.at(n);symbol(n,"class",n,"rudolf "+n+(c->base.empty()?"":" diamond "+c->base),"",c->token);for(const auto& f:c->fields)symbol(f.name,"field",f.type,f.type+" "+n+"."+f.name,n,f.token,f.priv);if(c->ctor)fn(c->ctor);for(const auto& m:c->methods)fn(m.second);}
    return out+"]}";
}
}
