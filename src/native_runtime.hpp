#pragma once
#include "native_ops.hpp"
#include <csignal>
#include <tuple>
#include <type_traits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace uma_native {
struct Object : std::enable_shared_from_this<Object> {
    virtual ~Object()=default;
    virtual const char* name() const { return "array"; }
    virtual void clear() {}
    virtual void trace(const std::function<void(const std::shared_ptr<Object>&)>&) const {}
};
inline std::vector<std::weak_ptr<Object>> heap;
inline std::size_t nextCollection=1024;
// Trial deletion: roots are references from outside the registered object graph.
inline void collect(){
    std::vector<std::shared_ptr<Object>> live;
    for(auto& w:heap)if(auto o=w.lock())live.push_back(std::move(o));
    std::unordered_map<Object*,std::size_t> ids;
    for(std::size_t i=0;i<live.size();++i)ids[live[i].get()]=i;
    std::vector<std::size_t> incoming(live.size(),0),work;
    std::vector<bool> marked(live.size(),false);
    for(auto& o:live)o->trace([&](const std::shared_ptr<Object>& v){auto it=ids.find(v.get());if(it!=ids.end())++incoming[it->second];});
    for(std::size_t i=0;i<live.size();++i)if(live[i].use_count()>static_cast<long>(incoming[i]+1)){marked[i]=true;work.push_back(i);}
    while(!work.empty()){auto i=work.back();work.pop_back();live[i]->trace([&](const std::shared_ptr<Object>& v){auto it=ids.find(v.get());if(it!=ids.end() && !marked[it->second]){marked[it->second]=true;work.push_back(it->second);}});}
    for(std::size_t i=0;i<live.size();++i)if(!marked[i])live[i]->clear();
    live.clear();heap.erase(std::remove_if(heap.begin(),heap.end(),[](const auto& w){return w.expired();}),heap.end());
    nextCollection=std::max<std::size_t>(1024,heap.size()*2+1);
}
inline void track(const std::shared_ptr<Object>& o){if(heap.size()>=nextCollection)collect();heap.push_back(o);}
template<class T> void traceValue(const T&,const std::function<void(const std::shared_ptr<Object>&)>&){}
template<class T> void traceValue(const std::shared_ptr<T>& v,const std::function<void(const std::shared_ptr<Object>&)>& visit){if(v)visit(std::dynamic_pointer_cast<Object>(v));}
template<class S> struct Fn;
template<class S> struct Callable;
template<class R,class... A> struct Callable<R(A...)>:Object {virtual R invoke(A...)=0;};
template<class S,class F,class... C> struct Closure;
template<class R,class... A,class F,class... C> struct Closure<R(A...),F,C...>:Callable<R(A...)> {
    F function;std::tuple<C...> captures;
    Closure(F f,C... c):function(std::move(f)),captures(std::move(c)...){}
    R invoke(A... args)override{return std::apply([&](auto&... captured)->R{return function(captured...,args...);},captures);}
    void trace(const std::function<void(const std::shared_ptr<Object>&)>& visit)const override{std::apply([&](const auto&... c){(traceValue(c,visit),...);},captures);}
    void clear()override{std::apply([](auto&... c){((c={}),...);},captures);}
};
template<class R,class... A> struct Fn<R(A...)> {
    std::shared_ptr<Callable<R(A...)>> value;
    Fn()=default;
    template<class F,class=std::enable_if_t<!std::is_same_v<std::decay_t<F>,Fn>>> Fn(F function){value=std::make_shared<Closure<R(A...),F>>(function);track(value);}
    R operator()(A... args)const{if(!value)error({"<funcao>",1,1},"Chamada de funcao nao inicializada.");return value->invoke(args...);}
};
template<class S> void traceValue(const Fn<S>& f,const std::function<void(const std::shared_ptr<Object>&)>& visit){if(f.value)visit(f.value);}
template<class S,class F,class... C> Fn<S> makeClosure(F function,C... captures){Fn<S> result;result.value=std::make_shared<Closure<S,F,C...>>(function,std::move(captures)...);track(result.value);return result;}
inline volatile std::sig_atomic_t stopped=0;
inline void interrupt(int) { stopped=1; }
inline void tick(Loc l) { if(stopped)error(l,"Execucao interrompida."); }
struct Session {
    Session(){configureStack();std::signal(SIGINT,interrupt);}
    ~Session(){std::vector<std::shared_ptr<Object>> live;for(auto& w:heap)if(auto o=w.lock())live.push_back(o);for(auto& o:live)o->clear();heap.clear();}
};
inline std::size_t depth=0;
struct Frame {
    Loc at;
    explicit Frame(Loc l):at(l){if(++depth>4096){--depth;error(l,"Limite de chamadas atingido (4096).");}tick(l);}
    ~Frame(){--depth;}
};
template<class T> struct Cell { T value; };
template<class T> struct Array : Object { std::vector<Cell<T>> values;void clear() override {values.clear();} void trace(const std::function<void(const std::shared_ptr<Object>&)>& visit)const override{for(const auto& c:values)traceValue(c.value,visit);} };
template<class T> using A=std::shared_ptr<Array<T>>;
template<class T> A<T> array(std::initializer_list<T> values={}) {auto a=std::make_shared<Array<T>>();for(const auto& v:values)a->values.push_back({v});track(a);return a;}
template<class T> T* require(const std::shared_ptr<T>& p,Loc l) {if(!p)error(l,"Acesso a nature.");return p.get();}
template<class T> struct Slot {
    T value{};bool initialized=false;
    T& get(Loc l){if(!initialized)error(l,"Variavel global ou campo ainda nao inicializado.");return value;}
};
template<class T> struct FieldSlot {T value{};T& get(Loc){return value;}};
template<class T> struct Ref {T* p;std::shared_ptr<Object> owner;T read()const{return *p;} T& get()const{return *p;}T write(T v)const{*p=v;return v;}};
template<class T> Ref<T> ref(T& value){return {&value,{}};}
template<class O,class T> Ref<T> field(std::shared_ptr<O> o,Slot<T> O::*m,Loc l){auto p=require(o,l);return {&((p->*m).get(l)),o};}
template<class O,class T> Ref<T> field(std::shared_ptr<O> o,FieldSlot<T> O::*m,Loc l){auto p=require(o,l);return {&((p->*m).get(l)),o};}
template<class T> struct ArrayRef {
    A<T> owner;std::size_t i;Loc at;
    T& get() const {auto& v=require(owner,at)->values;if(i>=v.size())error(at,"Array mudou durante a expressao.");return v[i].value;}
    T read() const {auto& v=require(owner,at)->values;if(i>=v.size())error(at,"Array mudou durante a expressao.");return v[i].value;}
    T write(T value) const {auto& v=require(owner,at)->values;if(i>=v.size())error(at,"Array mudou durante a expressao.");v[i].value=value;return value;}
};
template<class T> ArrayRef<T> element(A<T> a,I i,Loc l){auto p=require(a,l);return {a,index(i,p->values.size(),l),l};}
inline char32_t at(const Text& s,I i,Loc l){return character(s.at(i,l).str(),l);}
inline std::string format(std::nullptr_t,int=0){return "nature";}
inline std::string format(const std::string& s,int=0){return s;}
inline std::string format(const Text& s,int=0){return s.str();}
inline std::string format(I i,int=0){return std::to_string(i);}
inline std::string format(double d,int=0){std::ostringstream s;s<<std::setprecision(17)<<d;return s.str();}
inline std::string format(bool b,int=0){return b?"turf":"dirt";}
inline std::string format(char32_t c,int=0){return encode(c);}
template<class T> std::string format(const std::shared_ptr<T>& o,int=0){return o?std::string("<")+o->name()+">":"nature";}
template<class T> std::string format(const A<T>& a,int nesting=0){if(!a)return "nature";if(nesting>=32)return "[...]";std::string s="[";for(std::size_t i=0;i<a->values.size();++i){if(i)s+=", ";s+=format(a->values[i].value,nesting+1);}return s+"]";}
template<class... T> void print(const T&... args){std::size_t i=0;((std::cout<<(i++?" ":"")<<format(args)),...);std::cout<<'\n';}
inline std::string input(Loc l){std::string s;if(!std::getline(std::cin,s))error(l,"suzuka encontrou fim da entrada.");if(!s.empty() && s.back()=='\r')s.pop_back();boundaries(s,l);return s;}
inline void assertion(bool b,Loc l,const std::string& message="verificacao falhou."){if(!b)error(l,"flash: "+message);}
inline I length(const Text& s,Loc l){return static_cast<I>(s.length(l));}
template<class T> I length(const A<T>& a,Loc l){return static_cast<I>(require(a,l)->values.size());}
template<class T,class V> void push(const A<T>& a,V v,Loc l){require(a,l)->values.push_back({static_cast<T>(v)});}
template<class T> T pop(const A<T>& a,Loc l){auto p=require(a,l);if(p->values.empty())error(l,"dober nao pode retirar de array vazio.");T v=p->values.back().value;p->values.pop_back();return v;}
inline Text encodeBytes(const A<I>& a,Loc l){std::string bytes;for(auto& c:require(a,l)->values){if(c.value<0 || c.value>255)error(l,"Byte fora do intervalo.");bytes+=char(c.value);}return base64Encode(bytes);}
inline A<I> decodeBytes(const Text& s,Loc l){auto a=array<I>();for(unsigned char c:base64Decode(s.str(),l))a->values.push_back({static_cast<I>(c)});return a;}
inline A<Text> argsArray(){auto a=array<Text>();for(const auto& s:arguments)a->values.push_back({Text(s)});return a;}
inline A<Text> directoryArray(const Text& path,Loc l){auto a=array<Text>();for(auto& s:listDir(path.str(),l))a->values.push_back({Text(std::move(s))});return a;}
template<class T> void sortArray(const A<T>& a,Loc l){auto& v=require(a,l)->values;std::sort(v.begin(),v.end(),[](const auto& x,const auto& y){return x.value<y.value;});}
template<class T,class F> void sortArray(const A<T>& a,F compare,Loc l){auto original=require(a,l);auto v=original->values;std::stable_sort(v.begin(),v.end(),[&](const auto& x,const auto& y){return compare(x.value,y.value);});original->values=std::move(v);}
inline char32_t ticket(I v,Loc l){if(v<0 || v>0x10ffff || !scalar(static_cast<std::uint32_t>(v)))error(l,"ticket exige um codigo Unicode valido.");return static_cast<char32_t>(v);}
inline A<I> readBytes(const std::string& path,Loc l){auto a=array<I>();for(unsigned char c:read(path,l,false))a->values.push_back({static_cast<I>(c)});return a;}
inline void writeBytes(const std::string& path,const A<I>& a,Loc l){std::string s;for(const auto& cell:require(a,l)->values){if(cell.value<0 || cell.value>255)error(l,"Byte fora do intervalo 0..255.");s+=static_cast<char>(cell.value);}write(path,s,l);}
inline Text concat(const Text& a,const Text& b){return a+b;}
inline Text concat(char32_t a,const Text& b){return Text(encode(a))+b;}
inline Text concat(const Text& a,char32_t b){return a+Text(encode(b));}
template<class R> Text append(R r,const Text& s){r.get().append(s);return r.read();}
template<class R> Text append(R r,char32_t c){return append(r,Text(encode(c)));}
template<class A,class B> bool equal(const A& a,const B& b){return a==b;}
inline bool equal(I a,double b){return compare(a,b)==0;}
inline bool equal(double a,I b){return compare(a,b)==0;}
}
