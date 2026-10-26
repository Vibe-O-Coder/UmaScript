#pragma once
#include <map>
namespace uma_native {
struct Json {
    enum Kind {Null,Bool,Number,String,Array,Object} kind=Null;
    std::string text;
    std::vector<Json> array;
    std::map<std::string,Json> object;
};
inline std::string jsonQuote(const std::string& s){
    const char* hex="0123456789abcdef";std::string out="\"";
    for(unsigned char c:s){if(c=='"' || c=='\\'){out+='\\';out+=char(c);}else if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out+'"';
}
inline std::string jsonEncode(const Json& j){
    switch(j.kind){case Json::Null:return "null";case Json::Bool:case Json::Number:return j.text;case Json::String:return jsonQuote(j.text);
    case Json::Array:{std::string s="[";for(const auto& v:j.array){if(s.size()>1)s+=',';s+=jsonEncode(v);}return s+"]";}
    case Json::Object:{std::string s="{";for(const auto& v:j.object){if(s.size()>1)s+=',';s+=jsonQuote(v.first)+":"+jsonEncode(v.second);}return s+"}";}}
    return "null";
}
struct JsonParser {
    const std::string& s;Loc loc;std::size_t pos=0;
    [[noreturn]] void bad(){error(loc,"JSON invalido no byte "+std::to_string(pos)+".");}
    void space(){while(pos<s.size() && (s[pos]==' ' || s[pos]=='\t' || s[pos]=='\r' || s[pos]=='\n'))++pos;}
    bool take(char c){space();if(pos<s.size() && s[pos]==c){++pos;return true;}return false;}
    unsigned hex4(){unsigned cp=0;for(int i=0;i<4;++i){if(pos>=s.size())bad();char c=s[pos++];int v=c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:c>='A' && c<='F'?c-'A'+10:-1;if(v<0)bad();cp=(cp<<4)|static_cast<unsigned>(v);}return cp;}
    std::string string(){
        if(!take('"'))bad();
        std::string out;
        while(pos<s.size()){unsigned char c=static_cast<unsigned char>(s[pos++]);if(c=='"')return out;if(c<32)bad();if(c!='\\'){out+=char(c);continue;}
            if(pos>=s.size())bad();
            char e=s[pos++];
            switch(e){case '"':case '\\':case '/':out+=e;
            break;
            case 'b':out+='\b';
            break;
            case 'f':out+='\f';
            break;
            case 'n':out+='\n';
            break;
            case 'r':out+='\r';
            break;
            case 't':out+='\t';
            break;
            case 'u':{auto cp=hex4();if(cp>=0xd800 && cp<=0xdbff){if(pos+2>s.size() || s[pos++]!='\\' || s[pos++]!='u')bad();auto low=hex4();if(low<0xdc00 || low>0xdfff)bad();cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);}if(!scalar(cp))bad();out+=encode(static_cast<char32_t>(cp));break;}default:bad();}
        }bad();
    }
    Json value(int depth=0){
        if(depth>128)error(loc,"JSON excede 128 niveis.");
        space();
        if(pos>=s.size())bad();
        Json j;
        char c=s[pos];
        if(c=='"'){j.kind=Json::String;j.text=string();return j;}
        if(take('{')){j.kind=Json::Object;if(take('}'))return j;do{auto key=string();if(!take(':'))bad();auto child=value(depth+1);if(!j.object.emplace(key,std::move(child)).second)error(loc,"JSON contem chave duplicada: "+key);}while(take(','));if(!take('}'))bad();return j;}
        if(take('[')){j.kind=Json::Array;if(take(']'))return j;do{j.array.push_back(value(depth+1));}while(take(','));if(!take(']'))bad();return j;}
        for(const auto& literal:{std::string("null"),std::string("true"),std::string("false")})if(s.compare(pos,literal.size(),literal)==0){pos+=literal.size();j.kind=literal=="null"?Json::Null:Json::Bool;j.text=literal;return j;}
        auto start=pos;if(s[pos]=='-')++pos;if(pos>=s.size())bad();
        if(s[pos]=='0')++pos;else {if(s[pos]<'1' || s[pos]>'9')bad();while(pos<s.size() && s[pos]>='0' && s[pos]<='9')++pos;}
        if(pos<s.size() && s[pos]=='.'){++pos;auto n=pos;while(pos<s.size() && s[pos]>='0' && s[pos]<='9')++pos;if(pos==n)bad();}
        if(pos<s.size() && (s[pos]=='e' || s[pos]=='E')){++pos;if(pos<s.size() && (s[pos]=='+' || s[pos]=='-'))++pos;auto n=pos;while(pos<s.size() && s[pos]>='0' && s[pos]<='9')++pos;if(pos==n)bad();}
        j.kind=Json::Number;j.text=s.substr(start,pos-start);return j;
    }
};
inline Json jsonParse(const std::string& s,Loc l){if(s.size()>16u*1024u*1024u)error(l,"JSON excede 16 MiB.");boundaries(s,l);JsonParser p{s,l};auto j=p.value();p.space();if(p.pos!=s.size())p.bad();return j;}
inline std::string jsonNormalize(const std::string& s,Loc l){return jsonEncode(jsonParse(s,l));}
inline std::string jsonGet(const std::string& s,const std::string& key,Loc l){auto j=jsonParse(s,l);if(j.kind!=Json::Object)error(l,"JSON nao e um objeto.");auto it=j.object.find(key);if(it==j.object.end())error(l,"JSON: chave ausente: "+key);return jsonEncode(it->second);}
inline std::string jsonAt(const std::string& s,I i,Loc l){auto j=jsonParse(s,l);if(j.kind!=Json::Array)error(l,"JSON nao e um array.");return jsonEncode(j.array[index(i,j.array.size(),l)]);}
inline std::string jsonKind(const std::string& s,Loc l){auto j=jsonParse(s,l);static const char* names[]={"null","bool","number","string","array","object"};return names[j.kind];}
inline std::string jsonValue(const std::string& s,Loc l){auto j=jsonParse(s,l);if(j.kind==Json::Array || j.kind==Json::Object)error(l,"JSON nao e escalar.");return j.text;}
inline I jsonSize(const std::string& s,Loc l){auto j=jsonParse(s,l);if(j.kind==Json::Array)return static_cast<I>(j.array.size());if(j.kind==Json::Object)return static_cast<I>(j.object.size());error(l,"JSON nao e colecao.");}
inline std::string jsonSet(const std::string& s,const std::string& key,const std::string& value,Loc l){auto j=jsonParse(s,l);if(j.kind!=Json::Object)error(l,"JSON nao e objeto.");j.object[key]=jsonParse(value,l);return jsonEncode(j);}
inline std::string base64Encode(const std::string& data){const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;unsigned bits=0,count=0;for(unsigned char c:data){bits=(bits<<8)|c;count+=8;while(count>=6){count-=6;out+=alphabet[(bits>>count)&63];}}if(count)out+=alphabet[(bits<<(6-count))&63];while(out.size()%4)out+='=';return out;}
inline std::string base64Decode(const std::string& s,Loc l){
    if(s.size()%4)error(l,"Base64: tamanho invalido.");
    std::string out;
    auto digit=[&](char c){if(c>='A' && c<='Z')return c-'A';if(c>='a' && c<='z')return c-'a'+26;if(c>='0' && c<='9')return c-'0'+52;if(c=='+')return 62;if(c=='/')return 63;return -1;};
    for(std::size_t i=0;i<s.size();i+=4){int a=digit(s[i]),b=digit(s[i+1]),c=s[i+2]=='='?0:digit(s[i+2]),d=s[i+3]=='='?0:digit(s[i+3]);bool p2=s[i+2]=='=',p3=s[i+3]=='=';
        if(a<0 || b<0 || c<0 || d<0 || (p2 && !p3) || ((p2 || p3) && i+4!=s.size()) || (p2 && (b&15)) || (!p2 && p3 && (c&3)))error(l,"Base64 invalido.");
        unsigned v=(static_cast<unsigned>(a)<<18)|(static_cast<unsigned>(b)<<12)|(static_cast<unsigned>(c)<<6)|static_cast<unsigned>(d);out+=char(v>>16);if(!p2)out+=char(v>>8);if(!p3)out+=char(v);
    }return out;
}
}
