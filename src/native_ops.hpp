#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <functional>
#include <regex>
#include <random>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#else
#include <sys/resource.h>
#endif

namespace uma_native {
using I = std::int64_t;
struct Loc { const char* file; int line, col; };
struct Failure : std::runtime_error { Loc at; Failure(Loc l, const std::string& s):std::runtime_error(s),at(l){} };
[[noreturn]] inline void error(Loc l, const std::string& s) { throw Failure(l,s); }
template<class F> struct Finally {F action;bool armed=true;~Finally()noexcept(false){if(armed)action();}};
template<class F> Finally<F> finally(F f){return {f};}
inline I bits(std::uint64_t x) { I out; std::memcpy(&out,&x,8); return out; }
inline I bitop(I a,I b,const char* op,Loc l) {
    auto x=static_cast<std::uint64_t>(a), y=static_cast<std::uint64_t>(b);
    if(op[0]=='&')return bits(x&y);
    if(op[0]=='|')return bits(x|y);
    if(op[0]=='^')return bits(x^y);
    if(b<0 || b>63)error(l,"Deslocamento exige quantidade entre 0 e 63.");
    if(op[0]=='<')return bits(x<<b);
    return bits(b==0?x:((x>>b)|(a<0?(~std::uint64_t(0)<<(64-b)):0)));
}
inline I add(I a,I b,Loc l) {
#if defined(__GNUC__) || defined(__clang__)
    I result; if(__builtin_add_overflow(a,b,&result))error(l,"Overflow de speed (inteiro de 64 bits).");return result;
#else
    if((b>0 && a>INT64_MAX-b)||(b<0 && a<INT64_MIN-b))error(l,"Overflow de speed (inteiro de 64 bits).");return a+b;
#endif
}
inline I sub(I a,I b,Loc l) {
#if defined(__GNUC__) || defined(__clang__)
    I result; if(__builtin_sub_overflow(a,b,&result))error(l,"Overflow de speed (inteiro de 64 bits).");return result;
#else
    if((b<0 && a>INT64_MAX+b)||(b>0 && a<INT64_MIN+b))error(l,"Overflow de speed (inteiro de 64 bits).");return a-b;
#endif
}
inline I mul(I a,I b,Loc l) {
#if defined(__GNUC__) || defined(__clang__)
    I result; if(__builtin_mul_overflow(a,b,&result))error(l,"Overflow de speed (inteiro de 64 bits).");return result;
#else
    bool bad=a>0?(b>0?a>INT64_MAX/b:b<INT64_MIN/a):a<0?(b>0?a<INT64_MIN/b:b<0 && a<INT64_MAX/b):false;
    if(bad)error(l,"Overflow de speed (inteiro de 64 bits).");
    return a*b;
#endif
}
inline I divide(I a,I b,Loc l) { if(!b)error(l,"Divisao por zero.");if(a==INT64_MIN && b==-1)error(l,"Overflow de speed.");return a/b; }
inline I modulo(I a,I b,Loc l) { if(!b)error(l,"Divisao por zero.");return a==INT64_MIN && b==-1?0:a%b; }
inline double finite(double x,Loc l) { if(!std::isfinite(x))error(l,"Resultado stamina nao finito.");return x; }
inline double divide(double a,double b,Loc l) { if(b==0)error(l,"Divisao por zero.");return finite(a/b,l); }
inline bool scalar(std::uint32_t c) { return c<=0x10ffff && !(c>=0xd800 && c<=0xdfff); }
inline std::string encode(char32_t c) {
    std::string s;
    if(c<128)s+=char(c);
    else if(c<2048){s+=char(0xc0|(c>>6));s+=char(0x80|(c&63));}
    else if(c<65536){s+=char(0xe0|(c>>12));s+=char(0x80|((c>>6)&63));s+=char(0x80|(c&63));}
    else {s+=char(0xf0|(c>>18));s+=char(0x80|((c>>12)&63));s+=char(0x80|((c>>6)&63));s+=char(0x80|(c&63));}
    return s;
}
inline std::vector<std::size_t> boundaries(const std::string& s,Loc l) {
    std::vector<std::size_t> v;
    for(std::size_t i=0;i<s.size();) {
        v.push_back(i);auto c=static_cast<unsigned char>(s[i]);std::size_t n;std::uint32_t cp;
        if(c<128){n=1;cp=c;}else if((c&0xe0)==0xc0){n=2;cp=c&31;}else if((c&0xf0)==0xe0){n=3;cp=c&15;}else if((c&0xf8)==0xf0){n=4;cp=c&7;}else error(l,"Sequencia UTF-8 invalida.");
        if(i+n>s.size())error(l,"Sequencia UTF-8 incompleta.");
        for(std::size_t j=1;j<n;++j){auto d=static_cast<unsigned char>(s[i+j]);if((d&0xc0)!=0x80)error(l,"Sequencia UTF-8 invalida.");cp=(cp<<6)|(d&63);}
        if(!scalar(cp)||(n==2 && cp<128)||(n==3 && cp<2048)||(n==4 && cp<65536))error(l,"Caractere Unicode invalido.");
        i+=n;
    }
    v.push_back(s.size());return v;
}
// Copy-on-write UTF-8 values: copies share bytes and their Unicode index.
class Text {
    struct Data {
        std::string bytes;
        mutable bool indexed=false;
        mutable bool ascii=true;
        mutable std::vector<std::size_t> offsets;
        explicit Data(std::string s):bytes(std::move(s)){}
        void build(Loc l) const {
            if(indexed)return;
            ascii=std::all_of(bytes.begin(),bytes.end(),[](unsigned char c){return c<128;});
            if(!ascii)offsets=boundaries(bytes,l);
            indexed=true;
        }
    };
    std::shared_ptr<Data> p;
public:
    Text():p(std::make_shared<Data>("")){}
    Text(std::string s):p(std::make_shared<Data>(std::move(s))){}
    Text(const char* s):Text(std::string(s)){}
    Text(const char* s,std::size_t n):Text(std::string(s,n)){}
    operator const std::string&() const {return p->bytes;}
    const std::string& str()const{return p->bytes;}
    std::size_t size()const{return str().size();}
    bool empty()const{return str().empty();}
    char operator[](std::size_t n)const{return str()[n];}
    const char* c_str()const{return str().c_str();}
    const char* data()const{return str().data();}
    std::size_t find(const std::string& s,std::size_t pos=0)const{return str().find(s,pos);}
    std::string substr(std::size_t i,std::size_t n=std::string::npos)const{return str().substr(i,n);}
    int compare(const Text& s)const{return str().compare(s.str());}
    std::size_t length(Loc l)const{p->build(l);return p->ascii?size():p->offsets.size()-1;}
    std::size_t offset(std::size_t i,Loc l)const{p->build(l);return p->ascii?i:p->offsets.at(i);}
    Text at(I i,Loc l)const{
        auto n=length(l);if(i<0 || static_cast<std::uint64_t>(i)>=n)error(l,"Indice fora dos limites: "+std::to_string(i)+".");
        auto k=static_cast<std::size_t>(i),a=offset(k,l),b=offset(k+1,l);return substr(a,b-a);
    }
    void append(const Text& s){
        if(&s==this){Text snapshot=s;append(snapshot);return;}
        p->build({"<texto>",1,1});s.p->build({"<texto>",1,1});
        if(!p.unique())p=std::make_shared<Data>(*p);
        auto oldSize=size();
        if(!p->ascii || !s.p->ascii){
            if(p->ascii){p->offsets.reserve(oldSize+s.size()+1);for(std::size_t i=0;i<=oldSize;++i)p->offsets.push_back(i);}
            if(s.p->ascii){for(std::size_t i=1;i<=s.size();++i)p->offsets.push_back(oldSize+i);}
            else for(std::size_t i=1;i<s.p->offsets.size();++i)p->offsets.push_back(oldSize+s.p->offsets[i]);
            p->ascii=false;
        }
        p->bytes.append(s.str());
    }
    friend Text operator+(const Text& a,const Text& b){return a.str()+b.str();}
    friend std::string operator+(const char* a,const Text& b){return std::string(a)+b.str();}
    friend std::string operator+(std::string a,const Text& b){return a+b.str();}
    friend bool operator==(const Text& a,const Text& b){return a.str()==b.str();}
    friend bool operator!=(const Text& a,const Text& b){return !(a==b);}
    friend bool operator<(const Text& a,const Text& b){return a.str()<b.str();}
    friend bool operator>(const Text& a,const Text& b){return b<a;}
};
inline I wrapAdd(I a,I b){return bits(static_cast<std::uint64_t>(a)+static_cast<std::uint64_t>(b));}
inline I wrapSub(I a,I b){return bits(static_cast<std::uint64_t>(a)-static_cast<std::uint64_t>(b));
}
inline I wrapMul(I a,I b){return bits(static_cast<std::uint64_t>(a)*static_cast<std::uint64_t>(b));}
inline std::vector<std::string> arguments;
inline void configureStack(){
#ifndef _WIN32
    rlimit limits{};if(getrlimit(RLIMIT_STACK,&limits)==0){rlim_t desired=256u*1024u*1024u;if(limits.rlim_max!=RLIM_INFINITY)desired=std::min(desired,limits.rlim_max);if(limits.rlim_cur<desired){limits.rlim_cur=desired;setrlimit(RLIMIT_STACK,&limits);}}
#endif
}
inline std::vector<std::string> programArguments(int argc,char** argv){
    std::vector<std::string> result;
#ifdef _WIN32
    (void)argc;(void)argv;int count=0;auto wide=CommandLineToArgvW(GetCommandLineW(),&count);if(!wide)error({"<argumentos>",1,1},"Falha ao ler argumentos.");
    for(int i=1;i<count;++i){int n=WideCharToMultiByte(CP_UTF8,0,wide[i],-1,nullptr,0,nullptr,nullptr);std::string s(static_cast<std::size_t>(n),'\0');WideCharToMultiByte(CP_UTF8,0,wide[i],-1,s.data(),n,nullptr,nullptr);if(!s.empty())s.pop_back();result.push_back(std::move(s));}LocalFree(wide);
#else
    for(int i=1;i<argc;++i)result.emplace_back(argv[i]);
#endif
    return result;
}
struct Exit {int code;};
[[noreturn]] inline void exitProgram(I code,Loc l){if(code<0 || code>255)error(l,"Codigo de saida deve estar entre 0 e 255.");throw Exit{static_cast<int>(code)};}
inline bool inputEnd(){return std::cin.peek()==std::char_traits<char>::eof();}
inline std::string readAll(Loc l){std::ostringstream s;s<<std::cin.rdbuf();if(std::cin.bad())error(l,"Falha na entrada.");auto text=s.str();boundaries(text,l);return text;}
inline std::string replaceText(const std::string& s,const std::string& from,const std::string& to,Loc l){
    if(from.empty())error(l,"Texto de busca vazio.");
    std::string out;std::size_t start=0,pos;
    while((pos=s.find(from,start))!=std::string::npos){out.append(s,start,pos-start);out+=to;start=pos+from.size();}out.append(s,start,std::string::npos);return out;
}
inline std::string regexReplace(const std::string& s,const std::string& pattern,const std::string& value,Loc l){try{return std::regex_replace(s,std::regex(pattern),value);}catch(const std::regex_error& e){error(l,std::string("Regex invalida: ")+e.what());}}
inline std::size_t index(I i,std::size_t size,Loc l) { if(i<0 || static_cast<std::uint64_t>(i)>=size)error(l,"Indice fora dos limites: "+std::to_string(i)+".");return static_cast<std::size_t>(i); }
inline char32_t character(const std::string& s,Loc l) { auto v=boundaries(s,l);if(v.size()!=2)error(l,"guts exige exatamente um caractere Unicode.");auto c=static_cast<unsigned char>(s[0]);std::uint32_t cp=c<128?c:c<224?c&31:c<240?c&15:c&7;for(std::size_t i=1;i<s.size();++i)cp=(cp<<6)|(static_cast<unsigned char>(s[i])&63);return char32_t(cp); }
inline std::string pathText(const std::string& s,Loc l) { if(s.find('\0')!=std::string::npos)error(l,"Caminho ou nome contem byte nulo.");return s; }
inline std::string pathJoin(const std::string& a,const std::string& b,Loc l){return (std::filesystem::u8path(pathText(a,l))/std::filesystem::u8path(pathText(b,l))).lexically_normal().u8string();}
inline std::string read(const std::string& path,Loc l,bool text=true) {
    std::ifstream f(std::filesystem::u8path(pathText(path,l)),std::ios::binary);
    if(!f)error(l,"Nao foi possivel abrir: "+path);
    std::ostringstream out;out<<f.rdbuf();if(f.bad())error(l,"Falha na leitura: "+path);
    auto s=out.str();if(text)boundaries(s,l);return s;
}
inline void write(const std::string& path,const std::string& text,Loc l,bool append=false) {
    std::ofstream f(std::filesystem::u8path(pathText(path,l)),std::ios::binary|(append?std::ios::app:std::ios::trunc));
    if(!f)error(l,"Nao foi possivel gravar: "+path);
    f.write(text.data(),static_cast<std::streamsize>(text.size()));f.close();if(!f)error(l,"Falha na gravacao: "+path);
}
inline std::string slice(const std::string& s,I start,I count,Loc l) { auto b=boundaries(s,l);if(start<0 || count<0 || static_cast<std::uint64_t>(start)>b.size()-1 || static_cast<std::uint64_t>(count)>b.size()-1-static_cast<std::size_t>(start))error(l,"Intervalo de texto fora dos limites.");auto i=static_cast<std::size_t>(start);return s.substr(b[i],b[i+static_cast<std::size_t>(count)]-b[i]); }
inline I find(const std::string& s,const std::string& part,Loc l) { auto pos=s.find(part);if(pos==std::string::npos)return -1;return static_cast<I>(boundaries(s.substr(0,pos),l).size()-1); }
inline I hashText(const std::string& s){std::uint64_t h=14695981039346656037ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return static_cast<I>(h&INT64_MAX);}
inline std::string asciiCase(std::string s,bool upper){for(char& c:s){if(upper && c>='a' && c<='z')c=char(c-'a'+'A');else if(!upper && c>='A' && c<='Z')c=char(c-'A'+'a');}return s;}
inline void mkdir(const std::string& s,Loc l){std::error_code ec;std::filesystem::create_directories(std::filesystem::u8path(pathText(s,l)),ec);if(ec)error(l,"Falha ao criar diretorio: "+ec.message());}
inline bool removePath(const std::string& s,Loc l){std::error_code ec;bool ok=std::filesystem::remove(std::filesystem::u8path(pathText(s,l)),ec);if(ec)error(l,"Falha ao remover caminho: "+ec.message());return ok;}
inline std::vector<std::string> listDir(const std::string& s,Loc l){std::vector<std::string> out;try{for(const auto& item:std::filesystem::directory_iterator(std::filesystem::u8path(pathText(s,l))))out.push_back(item.path().filename().u8string());}catch(const std::filesystem::filesystem_error& e){error(l,e.what());}std::sort(out.begin(),out.end());return out;}
inline I randomInt(){std::random_device r;return bits((static_cast<std::uint64_t>(r())<<32)^r());}
inline I unixTime(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
inline I clock() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline void sleep(I ms,Loc l) { if(ms<0 || ms>INT64_MAX/1000000)error(l,"Duracao invalida para uma_sleep.");std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline std::string env(const std::string& name,Loc l) {
    pathText(name,l);
#ifdef _WIN32
    if(name.empty())return "";
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0);
    if(!count)error(l,"Nome de variavel de ambiente UTF-8 invalido.");
    std::wstring wide(static_cast<std::size_t>(count),L'\0');MultiByteToWideChar(CP_UTF8,0,name.data(),static_cast<int>(name.size()),wide.data(),count);
    DWORD size=GetEnvironmentVariableW(wide.c_str(),nullptr,0);if(!size)return "";
    std::vector<wchar_t> buffer(size);DWORD used=GetEnvironmentVariableW(wide.c_str(),buffer.data(),size);
    if(used>=size)error(l,"Variavel de ambiente mudou durante a leitura.");
    if(!used)return "";
    int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,buffer.data(),static_cast<int>(used),nullptr,0,nullptr,nullptr);if(!bytes)error(l,"Variavel de ambiente contem Unicode invalido.");
    std::string s(static_cast<std::size_t>(bytes),'\0');WideCharToMultiByte(CP_UTF8,0,buffer.data(),static_cast<int>(used),s.data(),bytes,nullptr,nullptr);return s;
#else
    auto p=std::getenv(name.c_str());std::string s=p?p:"";boundaries(s,l);return s;
#endif
}
inline bool exists(const std::string& path,Loc l) { std::error_code ec;bool b=std::filesystem::exists(std::filesystem::u8path(pathText(path,l)),ec);if(ec)error(l,"Falha ao consultar caminho: "+ec.message());return b; }
inline I toInt(I v,Loc) { return v; }
inline I toInt(double v,Loc l) { if(!std::isfinite(v)||v<-9223372036854775808.0||v>=9223372036854775808.0)error(l,"Conversao para speed fora do intervalo.");return static_cast<I>(v); }
inline I toInt(const std::string& s,Loc l) { try {std::size_t used;auto v=std::stoll(s,&used,10);if(used!=s.size())throw std::invalid_argument("suffix");return v;}catch(const std::exception&){error(l,"taiki: texto nao representa um speed valido.");} }
template<class T> inline double toReal(T v,Loc l) { return finite(static_cast<double>(v),l); }
inline double toReal(const std::string& s,Loc l) { try {std::size_t used;double v=std::stod(s,&used);if(used!=s.size()||!std::isfinite(v))throw std::invalid_argument("suffix");return v;}catch(const std::exception&){error(l,"creek: texto nao representa um stamina valido.");} }
inline double toReal(const Text& s,Loc l){return toReal(s.str(),l);}
inline int compare(I a,double b) { if(b<-9223372036854775808.0)return 1;if(b>=9223372036854775808.0)return -1;I i=static_cast<I>(b);return a<i?-1:a>i?1:b>std::trunc(b)?-1:b<std::trunc(b)?1:0; }
inline int compare(double a,I b) { return -compare(b,a); }
template<class A,class B> inline int compare(const A& a,const B& b) { return a<b?-1:a>b?1:0; }
}

#include "json_runtime.hpp"
