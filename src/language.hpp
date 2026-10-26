#pragma once
#include "native_ops.hpp"
// UmaScript 2.0: lexer, parser, verificacao estatica e valores do runtime.
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>
#include <variant>
#include <deque>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;
namespace uma {
struct Token { string text; string kind; int line = 1, col = 1; string file;size_t begin=0,end=0; Token(string text="",string kind="",int line=1,int col=1,string file=""):text(move(text)),kind(move(kind)),line(line),col(col),file(move(file)){} };
struct Error : runtime_error {
    Token at; string phase; vector<string> stack;
    string code()const{return phase=="sintaxe"?"U0001":phase=="tipo"?"U0002":phase=="modulo"?"U0003":phase=="propriedade"?"U0004":string(what()).find("Overflow")!=string::npos?"U0042":"U0100";}
    Error(Token t, string p, string msg) : runtime_error(move(msg)), at(move(t)), phase(move(p)) {}
};
[[noreturn]] void fail(const Token& t, const string& phase, const string& msg) { throw Error(t, phase, msg); }
#include "vocabulary.hpp"
struct NativeSignature { string result; vector<string> args; string documentation; };
const map<string, NativeSignature> standardNatives = {
    {"uma_json_normalize", {"wit", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_get", {"wit", {"wit","wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_at", {"wit", {"wit","speed"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_kind", {"wit", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_value", {"wit", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_size", {"speed", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_set", {"wit", {"wit","wit","wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_json_quote", {"wit", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_base64_encode", {"wit", {"speed[]"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_base64_decode", {"speed[]", {"wit"}, "Dados estruturados com validacao e erros capturaveis."}},
    {"uma_args", {"wit[]", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_eof", {"track", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_read_all", {"wit", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_exit", {"power", {"speed"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_stderr", {"power", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_hash", {"speed", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_replace", {"wit", {"wit","wit","wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_lower", {"wit", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_upper", {"wit", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_regex_replace", {"wit", {"wit","wit","wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_mkdir", {"power", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_remove", {"track", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_list_dir", {"wit[]", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_path_join", {"wit", {"wit","wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_basename", {"wit", {"wit"}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_random", {"speed", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_unix_time", {"speed", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_collect", {"power", {}, "Biblioteca 3.1; erros de execucao podem ser capturados."}},
    {"uma_sqrt", {"stamina", {"stamina"}, "Raiz quadrada; rejeita negativos e resultados nao finitos."}},
    {"uma_pow", {"stamina", {"stamina", "stamina"}, "Potencia real; rejeita resultados nao finitos."}},
    {"uma_sin", {"stamina", {"stamina"}, "Seno em radianos."}},
    {"uma_cos", {"stamina", {"stamina"}, "Cosseno em radianos."}},
    {"uma_floor", {"stamina", {"stamina"}, "Maior inteiro real menor ou igual ao argumento."}},
    {"uma_ceil", {"stamina", {"stamina"}, "Menor inteiro real maior ou igual ao argumento."}},
    {"uma_log", {"stamina", {"stamina"}, "Logaritmo natural; exige argumento positivo."}},
    {"uma_read_text", {"wit", {"wit"}, "Le arquivo UTF-8 inteiro; erro de abertura, leitura ou codificacao interrompe a execucao."}},
    {"uma_write_text", {"power", {"wit", "wit"}, "Grava UTF-8 substituindo o conteudo; falhas de E/S sao erros."}},
    {"uma_append_text", {"power", {"wit", "wit"}, "Acrescenta texto UTF-8 ao final do arquivo."}},
    {"uma_exists", {"track", {"wit"}, "Consulta se o caminho existe; erros de acesso sao reportados."}},
    {"uma_clock", {"speed", {}, "Tempo monotonicamente crescente em nanossegundos; origem nao especificada."}},
    {"uma_sleep", {"power", {"speed"}, "Suspende a thread por milissegundos nao negativos."}},
    {"uma_slice", {"wit", {"wit", "speed", "speed"}, "Substring por valores escalares Unicode: inicio inclusivo e quantidade; limites invalidos sao erros."}},
    {"uma_find", {"speed", {"wit", "wit"}, "Primeira ocorrencia, em valores escalares Unicode; -1 se ausente."}},
    {"uma_env", {"wit", {"wit"}, "Le variavel de ambiente; retorna texto vazio quando ausente."}},
    {"uma_read_bytes", {"speed[]", {"wit"}, "Le bytes de um arquivo em inteiros de 0 a 255."}},
    {"uma_write_bytes", {"power", {"wit", "speed[]"}, "Grava bytes; rejeita valores fora de 0..255."}}
};
const unordered_set<string> builtins = [] { unordered_set<string> names = {"bakushin", "suzuka", "oguri", "falcon", "dober", "taiki", "creek", "maruzensky", "hayahide", "ticket", "flash", "ordenar"}; for (const auto& x : standardNatives) names.insert(x.first); return names; }();

bool reserved(const string& s) { return primitives.count(s) || builtins.count(s) || keywords.count(s); }
bool functionType(const string& s){return s.rfind("fn(",0)==0;}
vector<string> functionParts(const string& s){vector<string> out;string part;int depth=0;for(size_t i=3;i+1<s.size();++i){char c=s[i];if(c=='|' && depth==0){out.push_back(part);part.clear();}else{part+=c;if(c=='(')++depth;if(c==')')--depth;}}out.push_back(part);return out;}
string signature(const string& result,const vector<string>& args){string s="fn("+result;for(auto& a:args)s+="|"+a;return s+")";}
bool arrayType(const string& s) { return s.size() > 2 && s.substr(s.size() - 2) == "[]"; }
string elementType(const string& s) { return s.substr(0, s.size() - 2); }
bool numeric(const string& s) { return s == "speed" || s == "stamina"; }
bool scalar(uint32_t cp) { return cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff); }
string encode(uint32_t cp) {
    string s;
    if (cp <= 0x7f) s += char(cp);
    else if (cp <= 0x7ff) { s += char(0xc0 | (cp >> 6)); s += char(0x80 | (cp & 63)); }
    else if (cp <= 0xffff) { s += char(0xe0 | (cp >> 12)); s += char(0x80 | ((cp >> 6) & 63)); s += char(0x80 | (cp & 63)); }
    else { s += char(0xf0 | (cp >> 18)); s += char(0x80 | ((cp >> 12) & 63)); s += char(0x80 | ((cp >> 6) & 63)); s += char(0x80 | (cp & 63)); }
    return s;
}
// Retorna os limites de cada caractere, incluindo o limite final.
vector<size_t> boundaries(const string& s, const Token& t) {
    vector<size_t> out;
    for (size_t i = 0; i < s.size();) {
        out.push_back(i); unsigned char c = static_cast<unsigned char>(s[i]); size_t n; uint32_t cp;
        if (c < 128) { n = 1; cp = c; }
        else if ((c & 0xe0) == 0xc0) { n = 2; cp = c & 31; }
        else if ((c & 0xf0) == 0xe0) { n = 3; cp = c & 15; }
        else if ((c & 0xf8) == 0xf0) { n = 4; cp = c & 7; }
        else fail(t, "texto", "Sequencia UTF-8 invalida.");
        if (i + n > s.size()) fail(t, "texto", "Sequencia UTF-8 incompleta.");
        for (size_t j = 1; j < n; ++j) {
            unsigned char d = static_cast<unsigned char>(s[i + j]);
            if ((d & 0xc0) != 0x80) fail(t, "texto", "Sequencia UTF-8 invalida.");
            cp = (cp << 6) | (d & 63);
        }
        if (!scalar(cp) || (n == 2 && cp < 128) || (n == 3 && cp < 2048) || (n == 4 && cp < 65536))
            fail(t, "texto", "Caractere Unicode invalido.");
        i += n;
    }
    out.push_back(s.size()); return out;
}
struct Lexer {
    string source; size_t pos = 0; int line = 1, col = 1; vector<Token> tokens;
    explicit Lexer(string s) : source(move(s)) { if (source.substr(0, 3) == "\xef\xbb\xbf") pos = 3; }
    char peek(size_t offset = 0) const { return pos + offset < source.size() ? source[pos + offset] : '\0'; }
    char take() { char c = source[pos++]; if (c == '\n') { ++line; col = 1; } else ++col; return c; }
    static bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
    static bool digit(char c) { return c >= '0' && c <= '9'; }
    vector<Token> run() {
        while (pos < source.size()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { take(); continue; }
            Token t{"", "op", line, col};t.begin=pos;
            if (c == '/' && peek(1) == '/') { while (pos < source.size() && peek() != '\n') take(); continue; }
            if (c == '/' && peek(1) == '*') {
                take(); take();
                while (pos < source.size() && !(peek() == '*' && peek(1) == '/')) take();
                if (pos == source.size()) fail(t, "sintaxe", "Comentario /* sem fechamento */.");
                take(); take(); continue;
            }
            if (letter(c)) {
                t.kind = "id"; while (letter(peek()) || digit(peek())) t.text += take();
            } else if (digit(c)) {
                t.kind="number";
                if(c=='0' && (peek(1)=='x' || peek(1)=='X' || peek(1)=='b' || peek(1)=='B')){
                    take();char prefix=take();int base=prefix=='x' || prefix=='X'?16:2;string digits;bool previous=false;
                    while(letter(peek()) || digit(peek())){char d=take();if(d=='_'){if(!previous)fail(t,"sintaxe","Separador numerico invalido.");previous=false;continue;}int v=d>='0' && d<='9'?d-'0':d>='a' && d<='f'?d-'a'+10:d>='A' && d<='F'?d-'A'+10:99;if(v>=base)fail(t,"sintaxe","Digito invalido no literal.");digits+=d;previous=true;}
                    if(!previous)fail(t,"sintaxe","Literal numerico incompleto.");
                    try{t.text=to_string(stoull(digits,nullptr,base));
                    }catch(const exception&){fail(t,"tipo","Literal fora do intervalo de speed.");
                    }
                }else{
                    auto digits=[&](){bool previous=false;while(digit(peek()) || peek()=='_'){char d=take();if(d=='_'){if(!previous)fail(t,"sintaxe","Separador numerico invalido.");previous=false;}else{t.text+=d;previous=true;}}if(!previous)fail(t,"sintaxe","Literal numerico incompleto.");};
                    digits();if(peek()=='.' && digit(peek(1))){t.text+=take();digits();}if(peek()=='e' || peek()=='E'){t.text+=take();if(peek()=='+' || peek()=='-')t.text+=take();digits();}
                }
            } else if (c == '"' || c == '\'') {
                char quote = take(); t.kind = quote == '"' ? "string" : "char"; bool closed = false;
                while (pos < source.size()) {
                    char d = take();
                    if (d == quote) { closed = true; break; }
                    if (d == '\n' || d == '\r') fail(t, "sintaxe", "Literal nao pode conter quebra de linha; use \\n.");
                    if (d == '\\') {
                        if (pos == source.size()) break;
                        char e = take();
                        if (e == 'n') t.text += '\n'; else if (e == 'r') t.text += '\r'; else if (e == 't') t.text += '\t';
                        else if (e == '0') t.text += '\0'; else if (e == '\\' || e == '\'' || e == '"') t.text += e;
                        else if (e == 'u') {
                            uint32_t cp = 0;
                            for (int i = 0; i < 4; ++i) {
                                char h = peek(); int n = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
                                if (n < 0) fail(t, "sintaxe", "Escape \\u exige quatro digitos hexadecimais.");
                                take(); cp = (cp << 4) | unsigned(n);
                            }
                            if (!scalar(cp)) fail(t, "sintaxe", "Escape Unicode nao representa um caractere valido.");
                            t.text += encode(cp);
                        } else fail(t, "sintaxe", "Escape desconhecido: \\" + string(1, e));
                    } else t.text += d;
                }
                if (!closed) fail(t, "sintaxe", "Literal sem aspas de fechamento.");
                auto b = boundaries(t.text, t);
                if (t.kind == "char" && b.size() != 2) fail(t, "sintaxe", "guts exige exatamente um caractere Unicode.");
            } else {
                string two; two += c; two += peek(1);
                static const unordered_set<string> pairs = {"==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "++", "--", "&=", "|=", "^=", "<<", ">>", "->", "+%", "-%", "*%"};
                if ((two == "<<" || two == ">>" || two=="+%" || two=="-%" || two=="*%") && peek(2) == '=') { t.text = two + "="; take(); take(); take(); }
                else if (pairs.count(two)) { t.text = two; take(); take(); }
                else if (string("{}()[];,.?:+-*/%!=<>&|^~").find(c) != string::npos) t.text += take();
                else fail(t, "sintaxe", "Caractere inesperado no codigo. Identificadores usam letras ASCII, numeros e _.");
            }
            t.end=pos;if(t.kind=="id"){auto alias=vocabularyAliases.find(t.text);if(alias!=vocabularyAliases.end())t.text=alias->second;}
            tokens.push_back(move(t));
        }
        tokens.push_back({"", "end", line, col}); return tokens;
    }
};

struct Expr; struct Stmt; struct Function; struct Class;
using E = shared_ptr<Expr>; using S = shared_ptr<Stmt>; using F = shared_ptr<Function>; using C = shared_ptr<Class>;
struct Expr {
    enum Kind { Literal, Var, Array, Unary, Binary, Assign, Call, Get, Index, New, Self, Super, Postfix, Move, Borrow, Lambda } kind;
    Token token; string name, type; E a, b; vector<E> args;F function;bool namedFunction=false;
    Expr(Kind k, Token t) : kind(k), token(move(t)) {}
};
struct Stmt {
    enum Kind { Block, Variable, Expression, If, While, For, ForEach, Return, Break, Continue, Try, Throw } kind;
    Token token; string name, type, mode; E expr, cond, update; S init, yes, no, cleanup; vector<S> body;
    Stmt(Kind k, Token t) : kind(k), token(move(t)) {}
};
struct Param { string type, name; Token token; string mode; };
struct Function { string name, result, owner; bool priv = false; Token token; vector<Param> params; S body; vector<E> baseArgs; bool explicitBase = false; };
struct Field { string type, name; bool priv; E init; Token token; };
struct Class { string name, base;vector<string> interfaces; Token token; vector<Field> fields; unordered_map<string, F> methods; F ctor; };
struct GenericTemplate {vector<string> params;vector<Token> tokens;};
struct Program {unordered_map<string,C> interfaces;map<string,GenericTemplate> templates;map<string,string> specializations; unordered_map<string, F> functions; unordered_map<string, C> classes; vector<string> classOrder, functionOrder; vector<S> statements; };
struct Parser {
    vector<Token> ts; size_t pos = 0; int depth = 0; Program p;
    explicit Parser(vector<Token> t) : ts(move(t)) {}
    const Token& peek(size_t off = 0) const { return ts[min(pos + off, ts.size() - 1)]; }
    Token take() { return ts[pos++]; }
    bool is(const string& s) const { return peek().kind == "op" || peek().kind == "id" ? peek().text == s : false; }
    bool match(const string& s) { if (!is(s)) return false; ++pos; return true; }
    Token expect(const string& s) { if(s==">" && is(">>")){ts[pos].text=">";auto t=ts[pos];++t.col;ts.insert(ts.begin()+static_cast<ptrdiff_t>(pos+1),t);}if (!is(s)) fail(peek(), "sintaxe", "Esperado '" + s + "'."); return take(); }
    Token name() { if (peek().kind != "id" || reserved(peek().text)) fail(peek(), "sintaxe", "Esperado um nome livre (palavras reservadas nao podem ser nomes)."); return take(); }
    string type() {
        if(match("tatica")){expect("(");vector<string> ps;if(!is(")"))do{ps.push_back(type());}while(match(","));expect(")");expect("->");auto result=type();return signature(result,ps);}
        if (peek().kind != "id" || (reserved(peek().text) && !primitives.count(peek().text))) fail(peek(), "sintaxe", "Esperado tipo explicito: speed, stamina, guts, wit, track ou uma classe.");
        string out = take().text;if(p.templates.count(out) && match("<")){vector<string> args;do{args.push_back(type());}while(match(","));expect(">");out=instantiate(out,args);}while (match("[")) { expect("]"); out += "[]"; } return out;
    }
    bool declaration() const {
        if(is("tatica")){size_t i=1;int d=0;do{if(peek(i).text=="(")++d;if(peek(i).text==")")--d;++i;}while(d>0 && peek(i).kind!="end");return peek(i).text=="->";}
        if (peek().kind != "id" || (reserved(peek().text) && !primitives.count(peek().text))) return false;
        size_t i=1;if(peek(i).text=="<"){int d=0;do{if(peek(i).text=="<")++d;if(peek(i).text==">")--d;if(peek(i).text==">>")d-=2;++i;}while(d>0 && peek(i).kind!="end");}while (peek(i).text == "[" && peek(i + 1).text == "]") i += 2;
        return peek(i).kind == "id" && !reserved(peek(i).text);
    }
    string qualifier(){if(match("own"))return "own";if(match("borrow"))return match("mut")?"borrow mut":"borrow";if(match("trofeu"))return "const";return "";}
    vector<Param> params() {
        vector<Param> out; expect("(");
        if (!is(")")) do { string mode=qualifier();string ty = type(); Token t = name(); out.push_back({ty, t.text, t,mode}); } while (match(","));
        expect(")"); return out;
    }
    F function(string ty, Token n, string owner = "", bool priv = false) {
        auto f = make_shared<Function>(); f->name = n.text; f->token = n; f->result = move(ty); f->owner = move(owner); f->priv = priv; f->params = params();
        f->body = block(); return f;
    }
    void templateDecl(const Token& name,size_t start){
        GenericTemplate g;expect("<");do{g.params.push_back(this->name().text);}while(match(","));expect(">");
        size_t after=pos;int braces=0;bool body=false;
        while(peek().kind!="end"){auto t=take();if(t.text=="{"){++braces;body=true;}if(t.text=="}" && --braces==0)break;}
        if(!body || braces)fail(name,"sintaxe","Template sem corpo completo.");
        for(size_t i=start;i<after;++i){if(ts[i].text=="<")break;g.tokens.push_back(ts[i]);}
        g.tokens.insert(g.tokens.end(),ts.begin()+static_cast<ptrdiff_t>(after),ts.begin()+static_cast<ptrdiff_t>(pos));
        g.tokens.push_back({"","end",name.line,name.col,name.file});
        if(!p.templates.emplace(name.text,move(g)).second)fail(name,"tipo","Template duplicado: "+name.text);
    }
    string instantiate(const string& name,const vector<string>& args){
        auto g=p.templates.at(name);if(args.size()!=g.params.size())fail(peek(),"tipo","Quantidade incorreta de parametros genericos.");
        string key=name;for(auto& a:args)key+="|"+a;auto existing=p.specializations.find(key);if(existing!=p.specializations.end())return existing->second;
        if(p.specializations.size()>=1024)fail(peek(),"tipo","Limite de 1024 especializacoes genericas.");
        string generated="_G_"+name;const char* hex="0123456789abcdef";for(auto& a:args){generated+="_";for(unsigned char c:a){generated+=hex[c>>4];generated+=hex[c&15];}}
        p.specializations[key]=generated;vector<Token> tokens;
        for(size_t tokenIndex=0;tokenIndex<g.tokens.size();++tokenIndex){auto t=g.tokens[tokenIndex];bool substituted=false;for(size_t i=0;i<args.size();++i)if(t.kind=="id" && t.text==g.params[i]){auto more=Lexer(args[i]).run();more.pop_back();for(auto& m:more){m.line=t.line;m.col=t.col;m.file=t.file;tokens.push_back(m);}substituted=true;break;}
            if(substituted)continue;
            if(t.kind=="id" && t.text==name && (tokenIndex+1==g.tokens.size() || g.tokens[tokenIndex+1].text!="<"))t.text=generated;
            tokens.push_back(t);
        }
        Parser parser(move(tokens));parser.p=p;p=parser.run();return generated;
    }
    void interfaceDecl(){
        auto at=expect("estilo"),n=name();auto c=make_shared<Class>();c->name=n.text;c->token=n;expect("{");
        while(!is("}")){match("condor");auto result=type();auto method=name();auto f=make_shared<Function>();f->name=method.text;f->result=result;f->token=method;f->owner=n.text;f->params=params();expect(";");if(!c->methods.emplace(f->name,f).second)fail(method,"tipo","Metodo duplicado na contract.");}
        expect("}");if(p.classes.count(n.text) || p.functions.count(n.text) || !p.interfaces.emplace(n.text,c).second)fail(at,"tipo","Nome global duplicado.");
    }
    void classDecl() {
        size_t start=pos;Token at = expect("rudolf"), n = name();if(is("<")){templateDecl(n,start);return;}auto c = make_shared<Class>(); c->name = n.text; c->token = n;
        if (match("diamond")) c->base = type();
        if(match("adota"))do{c->interfaces.push_back(type());
        }while(match(","));
        expect("{"); unordered_set<string> used;
        while (!is("}")) {
            if (peek().kind == "end") fail(at, "sintaxe", "Classe sem fechamento }.");
            bool priv = false; if (match("groove")) priv = true; else match("condor");
            if (is("opera")) {
                Token ct = take(); if (c->ctor) fail(ct, "sintaxe", "A classe ja possui um construtor opera.");
                auto f = make_shared<Function>(); f->name = "opera"; f->result = "power"; f->owner = c->name; f->priv = priv; f->token = ct; f->params = params();
                if (match(":")) { expect("mcqueen"); expect("("); f->baseArgs = arguments(")"); f->explicitBase = true; }
                f->body = block(); c->ctor = f; continue;
            }
            string ty = type(); Token member = name();
            if (!used.insert(member.text).second) fail(member, "sintaxe", "Membro duplicado: " + member.text);
            if (is("(")) c->methods[member.text] = function(ty, member, c->name, priv);
            else { E init; if (match("=")) init = expression(); expect(";"); c->fields.push_back({ty, member.text, priv, init, member}); }
        }
        expect("}"); match(";");
        if (p.classes.count(c->name) || p.functions.count(c->name)) fail(n, "sintaxe", "Nome global duplicado: " + c->name);
        p.classOrder.push_back(c->name); p.classes[c->name] = c;
    }
    Program run() {
        while (peek().kind != "end") {
            if(is("estilo")){interfaceDecl();continue;}
            if (is("rudolf")) { classDecl(); continue; }
            if(is("own") || is("borrow") || is("trofeu")){p.statements.push_back(statement());continue;}
            if (declaration()) {
                size_t start=pos;string ty = type(); Token n = name();
                if(is("<")){templateDecl(n,start);continue;}
                if (is("(")) {
                    auto f = function(ty, n);
                    if (p.functions.count(n.text) || p.classes.count(n.text)) fail(n, "sintaxe", "Nome global duplicado: " + n.text);
                    p.functionOrder.push_back(n.text); p.functions[n.text] = f;
                } else {auto s=variable(ty,n,true);if(s->mode=="group")p.statements.insert(p.statements.end(),s->body.begin(),s->body.end());else p.statements.push_back(s);}
            } else p.statements.push_back(statement());
        }
        return p;
    }
    S variable(string ty, Token n, bool semi) {
        auto s = make_shared<Stmt>(Stmt::Variable, n); s->type = move(ty); s->name = n.text;
        if (match("=")) s->expr = expression();
        if(semi && match(",")){auto group=make_shared<Stmt>(Stmt::Block,n);group->mode="group";group->body.push_back(s);do{auto name=this->name();group->body.push_back(variable(s->type,name,false));}while(match(","));expect(";");return group;}
        if (semi) expect(";");
        return s;
    }
    S block() {
        Token t = expect("{"); auto s = make_shared<Stmt>(Stmt::Block, t);
        while (!is("}")) { if (peek().kind == "end") fail(t, "sintaxe", "Bloco sem fechamento }."); s->body.push_back(statement()); }
        expect("}"); return s;
    }
    S statement() {
        if (++depth > 256) fail(peek(), "sintaxe", "Aninhamento sintatico excede 256 niveis.");
        struct Pop { int& n; ~Pop() { --n; } } pop{depth};
        Token t = peek();
        if (is("{")) return block();
        if(is("own") || is("borrow") || is("trofeu")){auto mode=qualifier();auto ty=type();auto n=name();auto s=variable(ty,n,true);s->mode=mode;if(!s->expr)fail(n,"propriedade","Declaracao qualificada exige inicializador.");return s;}
        if(match("treino")){
            auto s=make_shared<Stmt>(Stmt::Try,t);s->yes=block();
            if(match("tropeco")){expect("(");auto ty=type();if(ty!="wit")fail(peek(),"tipo","tropeco recebe wit com a mensagem do erro.");auto n=name();s->name=n.text;expect(")");s->no=block();}
            if(match("desaquecer"))s->cleanup=block();
            if(!s->no && !s->cleanup)fail(t,"sintaxe","treino exige tropeco ou desaquecer.");
            return s;
        }
        if(match("desclassificar")){auto s=make_shared<Stmt>(Stmt::Throw,t);s->expr=expression();expect(";");return s;}
        if (match("scarlet")) {
            auto s = make_shared<Stmt>(Stmt::If, t); expect("("); s->cond = expression(); expect(")"); s->yes = block();
            if (match("vodka")) s->no = is("scarlet") ? statement() : block();
            return s;
        }
        if (match("goldship")) { auto s = make_shared<Stmt>(Stmt::While, t); expect("("); s->cond = expression(); expect(")"); s->yes = block(); return s; }
        if (match("kitasan")) {
            auto s = make_shared<Stmt>(Stmt::For, t); expect("(");
            if (!is(";")) {
                if (declaration()) { string ty = type(); Token n = name();if(match("em")){s->kind=Stmt::ForEach;s->name=n.text;s->type=ty;s->expr=expression();expect(")");s->yes=block();return s;}s->init = variable(ty, n, false); }
                else { s->init = make_shared<Stmt>(Stmt::Expression, peek()); s->init->expr = expression(); }
            }
            expect(";"); if (!is(";")) s->cond = expression(); expect(";"); if (!is(")")) s->update = expression(); expect(")"); s->yes = block(); return s;
        }
        if (match("special")) { auto s = make_shared<Stmt>(Stmt::Return, t); if (!is(";")) s->expr = expression(); expect(";"); return s; }
        if (match("rice")) { auto s = make_shared<Stmt>(Stmt::Break, t); expect(";"); return s; }
        if (match("urara")) { auto s = make_shared<Stmt>(Stmt::Continue, t); expect(";"); return s; }
        if (declaration()) { string ty = type(); Token n = name(); return variable(ty, n, true); }
        auto s = make_shared<Stmt>(Stmt::Expression, t); if (!match(";")) { s->expr = expression(); expect(";"); } return s;
    }
    E node(Expr::Kind k, Token t, E a = {}, E b = {}) { auto e = make_shared<Expr>(k, move(t)); e->a = move(a); e->b = move(b); return e; }
    E expression() { return assign(); }
    E assign() {
        if (++depth > 256) fail(peek(), "sintaxe", "Expressao excede 256 niveis.");
        struct Pop { int& n; ~Pop() { --n; } } pop{depth};
        E a = logicalOr();
        if (is("=") || is("+=") || is("-=") || is("*=") || is("/=") || is("%=") || is("&=") || is("|=") || is("^=") || is("<<=") || is(">>=") || is("+%=") || is("-%=") || is("*%=")) { Token op = take(); return node(Expr::Assign, op, a, assign()); }
        return a;
    }
    E binary(E (Parser::*next)(), const unordered_set<string>& ops) {
        E a = (this->*next)(); while (peek().kind == "op" && ops.count(peek().text)) { Token t = take(); E b = (this->*next)(); a = node(Expr::Binary, t, a, b); } return a;
    }
    E logicalOr() { return binary(&Parser::logicalAnd, {"||"}); }
    E logicalAnd() { return binary(&Parser::bitOr, {"&&"}); }
    E bitOr() { return binary(&Parser::bitXor, {"|"}); }
    E bitXor() { return binary(&Parser::bitAnd, {"^"}); }
    E bitAnd() { return binary(&Parser::equality, {"&"}); }
    E equality() { return binary(&Parser::comparison, {"==", "!="}); }
    E comparison() { return binary(&Parser::shift, {"<", ">", "<=", ">="}); }
    E shift() { return binary(&Parser::term, {"<<", ">>"}); }
    E term() { return binary(&Parser::factor, {"+", "-", "+%", "-%"}); }
    E factor() { return binary(&Parser::unary, {"*", "/", "%", "*%"}); }
    E unary() {
        if (++depth > 256) fail(peek(), "sintaxe", "Expressao excede 256 niveis.");
        struct Pop { int& n; ~Pop() { --n; } } pop{depth};
        if(match("mover")){auto t=ts[pos-1];expect("(");auto a=expression();expect(")");return node(Expr::Move,t,a);}
        if(match("&")){auto t=ts[pos-1];auto e=node(Expr::Borrow,t);e->name=match("mut")?"mut":"";e->a=unary();return e;}
        if (is("!") || is("~") || is("-") || is("+") || is("++") || is("--")) { Token t = take(); return node(Expr::Unary, t, unary()); }
        if (match("tachyon")) { Token t = ts[pos - 1]; auto e = node(Expr::New, t); e->name = type(); expect("("); e->args = arguments(")"); return suffix(e); }
        return suffix(primary());
    }
    vector<E> arguments(const string& end) {
        vector<E> out; if (!is(end)) do { out.push_back(expression()); } while (match(",")); expect(end); return out;
    }
    E suffix(E a) {
        while (true) {
            if(a->kind==Expr::Var && p.templates.count(a->name) && match("<")){vector<string> args;do{args.push_back(type());}while(match(","));expect(">");a->name=instantiate(a->name,args);}
            if (match("(")) { auto e = node(Expr::Call, ts[pos - 1], a); e->args = arguments(")"); a = e; }
            else if (match(".")) { Token t = name(); auto e = node(Expr::Get, t, a); e->name = t.text; a = e; }
            else if (match("[")) { Token t = ts[pos - 1]; E i = expression(); expect("]"); a = node(Expr::Index, t, a, i); }
            else if (is("++") || is("--")) { Token t = take(); a = node(Expr::Postfix, t, a); }
            else break;
        }
        return a;
    }
    E primary() {
        Token t = take();
        if(t.text=="tatica" && t.kind=="id"){
            auto e=node(Expr::Lambda,t);auto f=make_shared<Function>();f->token=t;f->name="_lambda_"+to_string(t.line)+"_"+to_string(t.col)+"_"+to_string(p.functions.size());
            f->params=params();expect("->");f->result=type();f->body=block();e->function=f;return e;
        }
        if (t.kind == "number" || t.kind == "string" || t.kind == "char" || (t.kind == "id" && (t.text == "turf" || t.text == "dirt" || t.text == "nature"))) return node(Expr::Literal, t);
        if (t.kind == "id" && t.text == "teio") return node(Expr::Self, t);
        if (t.kind == "id" && t.text == "mcqueen") return node(Expr::Super, t);
        if (t.kind == "id" && (!reserved(t.text) || builtins.count(t.text))) { auto e = node(Expr::Var, t); e->name = t.text; return e; }
        if (t.kind == "op" && t.text == "(") { E e = expression(); expect(")"); return e; }
        if (t.kind == "op" && t.text == "[") { auto e = node(Expr::Array, t); e->args = arguments("]"); return e; }
        fail(t, "sintaxe", "Esperada uma expressao.");
    }
};

struct Member { string owner; Field* field = nullptr; F method; };
Member member(Program& p, string cls, const string& name) {
    while (!cls.empty()) {
        auto c = p.interfaces.count(cls)?p.interfaces.at(cls):p.classes.at(cls);
        for (auto& f : c->fields) if (f.name == name) return {cls, &f, {}};
        auto it = c->methods.find(name); if (it != c->methods.end()) return {cls, nullptr, it->second};
        cls = c->base;
    }
    return {};
}
bool subtype(const Program& p, string a, const string& b) {
    while(p.classes.count(a)){if(a==b)return true;auto c=p.classes.at(a);for(auto& name:c->interfaces)if(name==b)return true;a=c->base;}return false;
}
bool refType(const Program& p, const string& t) { return arrayType(t) || p.classes.count(t) || p.interfaces.count(t); }
bool accepts(const Program& p, const string& target, const string& value) {
    return target == value || (target == "stamina" && value == "speed") || (value == "nature" && refType(p, target)) || subtype(p, value, target);
}
struct Checker {
    Program& p; vector<unordered_map<string, string>> scopes{1}; string owner, result; int loops = 0, recursion = 0;bool inFinally=false;
    explicit Checker(Program& x) : p(x) {}
    void valid(const string& t, const Token& at, bool allowPower = false) {
        if(functionType(t)){auto parts=functionParts(t);valid(parts[0],at,true);for(size_t i=1;i<parts.size();++i)valid(parts[i],at);return;}
        if (arrayType(t)) { valid(elementType(t), at, false); return; }
        if ((!primitives.count(t) && !p.classes.count(t) && !p.interfaces.count(t)) || (t == "power" && !allowPower)) fail(at, "tipo", "Tipo invalido neste contexto: " + t);
    }
    void declare(const string& name, const string& ty, const Token& at) {
        if (p.functions.count(name) || p.classes.count(name) || !scopes.back().emplace(name, ty).second) fail(at, "tipo", "Nome duplicado ou em conflito com funcao/classe: " + name);
    }
    string lookup(const string& name, const Token& at) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) { auto f = it->find(name); if (f != it->end()) return f->second; }
        string suggestion;size_t best=3;
        auto distance=[](const string& a,const string& b){vector<size_t> row(b.size()+1);for(size_t i=0;i<row.size();++i)row[i]=i;for(size_t i=1;i<=a.size();++i){size_t old=row[0];row[0]=i;for(size_t j=1;j<=b.size();++j){size_t saved=row[j];row[j]=min({row[j]+1,row[j-1]+1,old+(a[i-1]!=b[j-1])});old=saved;}}return row.back();};
        for(auto& scope:scopes)for(auto& kv:scope){auto d=distance(name,kv.first);if(d<best){best=d;suggestion=kv.first;}}
        fail(at,"tipo","Variavel nao declarada: "+name+(suggestion.empty()?"":". Voce quis dizer '"+suggestion+"'?"));
    }
    void require(const string& expected, const string& actual, const Token& at) {
        if (!accepts(p, expected, actual)) fail(at, "tipo", "Esperado " + expected + ", recebido " + actual + ".");
    }
    void access(const Member& m, const Token& t) {
        if ((m.field ? m.field->priv : m.method && m.method->priv) && owner != m.owner) fail(t, "tipo", "Membro groove (privado) de " + m.owner + ".");
    }
    Member get(const E& e) {
        string ty = infer(e->a);
        if (!p.classes.count(ty) && !p.interfaces.count(ty)) fail(e->token, "tipo", "Acesso a membro exige uma instancia; recebido " + ty + ".");
        auto m = member(p, ty, e->name); if (m.owner.empty()) fail(e->token, "tipo", "Membro inexistente em " + ty + ": " + e->name);
        access(m, e->token); return m;
    }
    void args(const vector<E>& es, const vector<Param>& ps, const Token& t) {
        if (es.size() != ps.size()) fail(t, "tipo", "Esperados " + to_string(ps.size()) + " argumentos, recebidos " + to_string(es.size()) + ".");
        for (size_t i = 0; i < es.size(); ++i) require(ps[i].type, infer(es[i], ps[i].type), es[i]->token);
    }
    string op(const string& a, const string& b, const Token& t, const string& explicitOp = "") {
        string s = explicitOp.empty() ? t.text : explicitOp;
        if(s=="+%" || s=="-%" || s=="*%"){require("speed",a,t);require("speed",b,t);return "speed";}
        if (s == "&" || s == "|" || s == "^" || s == "<<" || s == ">>") { require("speed", a, t); require("speed", b, t); return "speed"; }
        if (s == "&&" || s == "||") { require("track", a, t); require("track", b, t); return "track"; }
        if (s == "==" || s == "!=") {
            if (a == "power" || b == "power" || (!a.empty() && a[0] == '@') || (!b.empty() && b[0] == '@') || !(a == b || (numeric(a) && numeric(b)) || accepts(p, a, b) || accepts(p, b, a))) fail(t, "tipo", "Comparacao incompativel entre " + a + " e " + b + ".");
            return "track";
        }
        if (s == "+" && ((a == "wit" && (b == "wit" || b == "guts")) || (b == "wit" && a == "guts"))) return "wit";
        if (s == "<" || s == ">" || s == "<=" || s == ">=") {
            if (!((numeric(a) && numeric(b)) || (a == "wit" && b == "wit") || (a == "guts" && b == "guts"))) fail(t, "tipo", "Ordenacao exige numeros ou textos do mesmo tipo.");
            return "track";
        }
        if (!numeric(a) || !numeric(b) || (s == "%" && (a != "speed" || b != "speed"))) fail(t, "tipo", "Operador " + s + " nao aceita " + a + " e " + b + ".");
        return a == "stamina" || b == "stamina" ? "stamina" : "speed";
    }
    string lvalue(const E& e) {
        if (e->kind == Expr::Var) return e->type = lookup(e->name, e->token);
        if (e->kind == Expr::Get) { auto m = get(e); if (m.field) return e->type = m.field->type; }
        if (e->kind == Expr::Index) { string t = infer(e->a); require("speed", infer(e->b), e->b->token); if (arrayType(t)) return e->type = elementType(t); }
        fail(e->token, "tipo", "Destino nao pode ser alterado. Use variavel, campo ou indice de array.");
    }
    string builtin(const string& n, const vector<E>& es, const Token& t) {
        auto count = [&](size_t low, size_t high) { if (es.size() < low || es.size() > high) fail(t, "tipo", n + ": quantidade incorreta de argumentos."); };
        auto standard = standardNatives.find(n);
        if (standard != standardNatives.end()) { const auto& sig = standard->second; count(sig.args.size(), sig.args.size()); for (size_t i = 0; i < es.size(); ++i) require(sig.args[i], infer(es[i], sig.args[i]), es[i]->token); return sig.result; }
        if(n=="ordenar"){count(1,2);auto a=infer(es[0]);if(!arrayType(a) || !(numeric(elementType(a)) || elementType(a)=="wit" || elementType(a)=="guts"))fail(t,"tipo","ordenar exige array de numeros ou texto.");if(es.size()==2){auto elem=elementType(a);require(signature("track",{elem,elem}),infer(es[1]),t);}return "power";}
        if (n == "bakushin") { for (auto& e : es) { string ty = infer(e); if (ty == "power" || ty[0] == '@') fail(e->token, "tipo", "bakushin exige valores imprimiveis."); } return "power"; }
        if (n == "suzuka") { count(0, 0); return "wit"; }
        if (n == "flash") { count(1, 2); require("track", infer(es[0]), t); if (es.size() == 2) require("wit", infer(es[1]), t); return "power"; }
        count(n == "falcon" ? 2 : 1, n == "falcon" ? 2 : 1); string a = infer(es[0]);
        if (n == "oguri") { if (!arrayType(a) && a != "wit") fail(t, "tipo", "oguri exige wit ou array."); return "speed"; }
        if (n == "falcon" || n == "dober") {
            if (!arrayType(a)) fail(t, "tipo", n + " exige um array.");
            if (n == "falcon") { require(elementType(a), infer(es[1], elementType(a)), t); return "power"; } return elementType(a);
        }
        if (n == "taiki" || n == "creek") { if (!numeric(a) && a != "wit") fail(t, "tipo", n + " exige speed, stamina ou wit."); return n == "taiki" ? "speed" : "stamina"; }
        if (n == "maruzensky") { if (a == "power" || a[0] == '@') fail(t, "tipo", "maruzensky exige um valor."); return "wit"; }
        if (n == "hayahide") { require("guts", a, t); return "speed"; }
        if (n == "ticket") { require("speed", a, t); return "guts"; }
        fail(t, "tipo", "Funcao nativa desconhecida.");
    }
    string infer(const E& e, const string& expected = "") {
        if(e->kind==Expr::Lambda && !e->type.empty())return e->type;
        if (++recursion > 512) fail(e->token, "tipo", "Expressao muito profunda.");
        struct Pop { int& n; ~Pop() { --n; } } pop{recursion};
        string t;
        switch (e->kind) {
        case Expr::Literal:
            if (e->token.kind == "number") {
                t = e->token.text.find_first_of(".eE") == string::npos ? "speed" : "stamina";
                try { if (t == "speed") stoll(e->token.text); else { double d = stod(e->token.text); if (!isfinite(d)) throw out_of_range("number"); } }
                catch (const exception&) { fail(e->token, "tipo", "Literal fora do intervalo de " + t + "."); }
            } else t = e->token.kind == "string" ? "wit" : e->token.kind == "char" ? "guts" : e->token.text == "nature" ? "nature" : "track";
            break;
        case Expr::Var: if (builtins.count(e->name)) t = "@native"; else if (p.functions.count(e->name)){auto f=p.functions.at(e->name);vector<string> args;for(auto& par:f->params)args.push_back(par.type);t=signature(f->result,args);e->namedFunction=true;} else t = lookup(e->name, e->token); break;
        case Expr::Lambda:{
            auto f=e->function;vector<string> types;for(auto& par:f->params)types.push_back(par.type);t=signature(f->result,types);
            vector<Param> captures;set<string> seen;for(auto& par:f->params)seen.insert(par.name);
            // Capture only referenced free variables, preserving lexical shadowing.
            set<string> used,locals;
            function<void(const E&)> ex;function<void(const S&)> st;
            ex=[&](const E& x){if(!x)return;if(x->kind==Expr::Var)used.insert(x->name);ex(x->a);ex(x->b);for(auto& a:x->args)ex(a);};
            st=[&](const S& x){if(!x)return;if(x->kind==Stmt::Variable)locals.insert(x->name);ex(x->expr);ex(x->cond);ex(x->update);st(x->init);st(x->yes);st(x->no);st(x->cleanup);for(auto& c:x->body)st(c);};st(f->body);
            for(auto it=scopes.rbegin();it!=scopes.rend();++it)for(auto& kv:*it)if(used.count(kv.first) && !locals.count(kv.first) && seen.insert(kv.first).second){captures.push_back({kv.second,kv.first,e->token,""});auto a=make_shared<Expr>(Expr::Var,e->token);a->name=kv.first;a->type=kv.second;e->args.push_back(a);}
            f->params.insert(f->params.begin(),captures.begin(),captures.end());auto outer=make_shared<Stmt>(Stmt::Block,e->token);outer->body.push_back(f->body);f->body=outer;
            e->name=f->name;if(p.functions.count(f->name))fail(e->token,"tipo","Lambda duplicada.");p.functions[f->name]=f;p.functionOrder.push_back(f->name);break;
        }
        case Expr::Move:case Expr::Borrow:t=infer(e->a,expected);break;
        case Expr::Self: if (owner.empty()) fail(e->token, "tipo", "teio so existe em metodos e inicializadores de campo."); t = owner; break;
        case Expr::Super: if (owner.empty() || p.classes.at(owner)->base.empty()) fail(e->token, "tipo", "mcqueen exige uma classe com heranca."); t = p.classes.at(owner)->base; break;
        case Expr::Array: {
            string elem = arrayType(expected) ? elementType(expected) : "";
            if (elem.empty()) {
                if (e->args.empty()) fail(e->token, "tipo", "Array vazio precisa de contexto tipado, como speed[] a = [].");
                elem = infer(e->args[0]);
                for (size_t i = 1; i < e->args.size(); ++i) { string v = infer(e->args[i]); if (numeric(elem) && numeric(v)) { if (v == "stamina") elem = "stamina"; } else require(elem, v, e->args[i]->token); }
                valid(elem, e->token);
            }
            for (auto& v : e->args) require(elem, infer(v, elem), v->token);
            t = elem + "[]"; break;
        }
        case Expr::Unary: {
            if (e->token.text == "-" && e->a->kind == Expr::Literal && e->a->token.kind == "number" && e->a->token.text == "9223372036854775808") { e->a->type = "speed"; t = "speed"; break; }
            string a = (e->token.text == "++" || e->token.text == "--") ? lvalue(e->a) : infer(e->a);
            if (e->token.text == "!") { require("track", a, e->token); t = "track"; } else if (e->token.text == "~") { require("speed", a, e->token); t = "speed"; } else { if (!numeric(a)) fail(e->token, "tipo", "Operador exige speed ou stamina."); t = a; } break;
        }
        case Expr::Postfix: t = lvalue(e->a); if (!numeric(t)) fail(e->token, "tipo", "++ e -- exigem numero."); break;
        case Expr::Binary: { string a = infer(e->a), b = infer(e->b); t = op(a, b, e->token); break; }
        case Expr::Assign: { string a = lvalue(e->a); e->a->type = a; string b = infer(e->b, a); if (e->token.text != "=") b = op(a, b, e->token, e->token.text.substr(0, e->token.text.size()-1)); require(a, b, e->token); t = a; break; }
        case Expr::Get: { auto m = get(e); t = m.field ? m.field->type : "@method"; break; }
        case Expr::Index: { string a = infer(e->a); require("speed", infer(e->b), e->token); if (arrayType(a)) t = elementType(a); else if (a == "wit") t = "guts"; else fail(e->token, "tipo", "Indice exige array ou wit."); break; }
        case Expr::New: {
            if (!p.classes.count(e->name)) fail(e->token, "tipo", "Classe nao declarada: " + e->name);
            auto ctor = p.classes.at(e->name)->ctor;
            if (ctor) { if (ctor->priv && owner != e->name) fail(e->token, "tipo", "Construtor groove (privado)."); args(e->args, ctor->params, e->token); }
            else if (!e->args.empty()) fail(e->token, "tipo", "Construtor implicito nao recebe argumentos.");
            t = e->name; break;
        }
        case Expr::Call: {
            if (e->a->kind == Expr::Var && builtins.count(e->a->name)) t = builtin(e->a->name, e->args, e->token);
            else {
                if(e->a->kind==Expr::Get && get(e->a).method){auto f=get(e->a).method;args(e->args,f->params,e->token);t=f->result;}
                else{auto fn=infer(e->a);if(!functionType(fn))fail(e->token,"tipo","Expressao nao e uma funcao.");auto parts=functionParts(fn);if(parts.size()!=e->args.size()+1)fail(e->token,"tipo","Quantidade incorreta de argumentos.");for(size_t i=0;i<e->args.size();++i)require(parts[i+1],infer(e->args[i],parts[i+1]),e->args[i]->token);t=parts[0];}
            }
            break;
        }
        }
        e->type = t; return t;
    }
    enum Flow { Fall = 1, Ret = 2, Brk = 4, Cont = 8 };
    int sequence(const vector<S>& ss) {
        int flow = Fall; for (auto& s : ss) { int next = statement(s); if (flow & Fall) flow = (flow & ~Fall) | next; } return flow;
    }
    int statement(const S& s, bool scope = true) {
        if(inFinally && (s->kind==Stmt::Return || s->kind==Stmt::Break || s->kind==Stmt::Continue))fail(s->token,"tipo","desaquecer nao permite retorno, rice ou urara.");
        switch (s->kind) {
        case Stmt::Block: {scope=scope && s->mode!="group"; if (scope) scopes.emplace_back(); int f = sequence(s->body); if (scope) scopes.pop_back(); return f; }
        case Stmt::Variable: valid(s->type, s->token); if (s->expr) require(s->type, infer(s->expr, s->type), s->token); declare(s->name, s->type, s->token); return Fall;
        case Stmt::Expression: if (s->expr) { string t = infer(s->expr); if (t[0] == '@') fail(s->token, "tipo", "Use (...) para chamar a funcao."); } return Fall;
        case Stmt::If: { require("track", infer(s->cond), s->token); int a = statement(s->yes), b = s->no ? statement(s->no) : Fall; return a | b; }
        case Stmt::ForEach:{
            auto collection=infer(s->expr);if(!arrayType(collection) && collection!="wit")fail(s->token,"tipo","em exige array ou texto.");
            require(s->type,collection=="wit"?"guts":elementType(collection),s->token);
            auto token=s->token;string name="_uma_foreach_"+to_string(token.line)+"_"+to_string(token.col),index=name+"_i";
            auto var=[&](string n){auto e=make_shared<Expr>(Expr::Var,token);e->name=move(n);return e;};
            auto literal=[&](string n){auto t=token;t.text=n;t.kind="number";return make_shared<Expr>(Expr::Literal,t);};
            auto seq=make_shared<Stmt>(Stmt::Variable,token);seq->name=name;seq->type=collection;seq->expr=s->expr;
            auto loop=make_shared<Stmt>(Stmt::For,token);loop->init=make_shared<Stmt>(Stmt::Variable,token);loop->init->name=index;loop->init->type="speed";loop->init->expr=literal("0");
            auto length=make_shared<Expr>(Expr::Call,token);length->a=var("oguri");length->args={var(name)};
            auto lt=token;lt.text="<";loop->cond=make_shared<Expr>(Expr::Binary,lt);loop->cond->a=var(index);loop->cond->b=length;
            auto inc=token;inc.text="++";loop->update=make_shared<Expr>(Expr::Postfix,inc);loop->update->a=var(index);
            auto item=make_shared<Stmt>(Stmt::Variable,token);item->name=s->name;item->type=s->type;item->expr=make_shared<Expr>(Expr::Index,token);item->expr->a=var(name);item->expr->b=var(index);
            loop->yes=s->yes;loop->yes->body.insert(loop->yes->body.begin(),item);s->kind=Stmt::Block;s->body={seq,loop};return statement(s);
        }
        case Stmt::While: case Stmt::For: {
            scopes.emplace_back(); if (s->init) statement(s->init); if (s->cond) require("track", infer(s->cond), s->token); if (s->update) infer(s->update);
            ++loops; int f = statement(s->yes); --loops; scopes.pop_back();
            bool forever = !s->cond || (s->cond->kind == Expr::Literal && s->cond->token.text == "turf");
            return (f & Ret) | ((!forever || (f & Brk)) ? Fall : 0);
        }
        case Stmt::Throw:require("wit",infer(s->expr),s->token);return Ret;
        case Stmt::Try:{int a=statement(s->yes),b=0;if(s->no){scopes.emplace_back();declare(s->name,"wit",s->token);b=statement(s->no,false);scopes.pop_back();}int c=Fall;if(s->cleanup){bool saved=inFinally;inFinally=true;c=statement(s->cleanup);inFinally=saved;}return (c&Fall)?(a|b):(c&~Fall);}
        case Stmt::Return:
            if (result.empty()) fail(s->token, "tipo", "special so pode aparecer em funcao/metodo.");
            if (result == "power") { if (s->expr) fail(s->token, "tipo", "Funcao power usa special; sem valor."); }
            else { if (!s->expr) fail(s->token, "tipo", "special exige um valor " + result + "."); require(result, infer(s->expr, result), s->token); } return Ret;
        case Stmt::Break: if (!loops) fail(s->token, "tipo", "rice exige um laco."); return Brk;
        case Stmt::Continue: if (!loops) fail(s->token, "tipo", "urara exige um laco."); return Cont;
        }
        return Fall;
    }
    void checkFunction(const F& f) {
        owner = f->owner; result = f->result; scopes.resize(1); scopes.emplace_back();
        for (auto& par : f->params) { valid(par.type, par.token); declare(par.name, par.type, par.token); }
        if (f->name == "opera") {
            auto c = p.classes.at(owner);
            if (c->base.empty() && f->explicitBase) fail(f->token, "tipo", "mcqueen no construtor exige diamond.");
            if (!c->base.empty()) {
                auto b = p.classes.at(c->base)->ctor;
                if (b && b->priv) fail(f->token, "tipo", "Construtor da base e groove (privado).");
                args(f->baseArgs, b ? b->params : vector<Param>{}, f->token);
            }
        }
        int flow = statement(f->body, false);
        if (f->result != "power" && (flow & Fall)) fail(f->token, "tipo", "Funcao " + f->name + " pode terminar sem retornar " + f->result + ".");
        scopes.resize(1); owner.clear(); result.clear();
    }
    void run() {
        for(auto& kv:p.interfaces)for(auto& method:kv.second->methods){auto f=method.second;valid(f->result,f->token,true);for(auto& param:f->params)valid(param.type,param.token);}
        // Valida primeiro o grafo para que buscas de membros nunca entrem em ciclo.
        for (auto& n : p.classOrder) {
            auto c = p.classes.at(n); unordered_set<string> seen; string base = n;
            while (!base.empty()) {
                if (!p.classes.count(base)) fail(c->token, "tipo", "Classe base inexistente: " + base);
                if (!seen.insert(base).second) fail(c->token, "tipo", "Ciclo de heranca detectado.");
                base = p.classes.at(base)->base;
            }
            for(auto& contract:c->interfaces){
                if(!p.interfaces.count(contract))fail(c->token,"tipo","Interface inexistente: "+contract);
                for(auto& pair:p.interfaces.at(contract)->methods){auto required=pair.second;auto found=member(p,n,pair.first);if(!found.method || found.method->priv || found.method->result!=required->result || found.method->params.size()!=required->params.size())fail(c->token,"tipo","Classe nao implementa "+contract+"."+pair.first);
                    for(size_t i=0;i<required->params.size();++i)if(found.method->params[i].type!=required->params[i].type || found.method->params[i].mode!=required->params[i].mode)fail(c->token,"tipo","Assinatura difere da contract: "+pair.first);
                }
            }
            for (auto& f : c->fields) {
                valid(f.type, f.token);
                if (!c->base.empty() && !member(p, c->base, f.name).owner.empty()) fail(f.token, "tipo", "Campo nao pode ocultar membro herdado: " + f.name);
            }
            for (auto& pair : c->methods) {
                auto f = pair.second; valid(f->result, f->token, true); for (auto& par : f->params) valid(par.type, par.token);
                if (!c->base.empty()) {
                    auto m = member(p, c->base, f->name);
                    if (!m.owner.empty()) {
                        if (m.field || m.method->priv) fail(f->token, "tipo", "Metodo nao pode ocultar campo ou metodo privado herdado.");
                        bool same = m.method->result == f->result && m.method->params.size() == f->params.size() && !f->priv;
                        if (same) for (size_t i = 0; i < f->params.size(); ++i) same = same && f->params[i].type == m.method->params[i].type;
                        if (!same) fail(f->token, "tipo", "Sobrescrita exige a mesma assinatura e visibilidade condor.");
                    }
                }
            }
            if (!c->ctor && !c->base.empty()) {
                auto b = p.classes.at(c->base)->ctor;
                if (b && (!b->params.empty() || b->priv)) fail(c->token, "tipo", "Declare opera e inicialize uma base acessivel com : mcqueen(...).");
            }
        }
        for (auto& name : p.functionOrder) { auto f = p.functions.at(name); valid(f->result, f->token, true); for (auto& par : f->params) valid(par.type, par.token); }
        sequence(p.statements);
        // Corpos veem as variaveis globais declaradas no programa; leituras precoces sao erros de execucao.
        for (auto& n : p.classOrder) {
            auto c = p.classes.at(n); owner = n;
            for (auto& f : c->fields) if (f.init) require(f.type, infer(f.init, f.type), f.token);
            owner.clear();
            if (c->ctor) checkFunction(c->ctor);
            for (auto& pair : c->methods) checkFunction(pair.second);
        }
        for(size_t i=0;i<p.functionOrder.size();++i){auto name=p.functionOrder[i];checkFunction(p.functions.at(name));}
    }
};

struct Object; struct ArrayValue; struct Bound;
struct Value {
    enum Tag { Void, Int, Double, Char, String, Bool, Null, Obj, Arr, Callable } tag=Void;
    union {unsigned char storage[16];int64_t i;double d;bool b;uma_native::Text s;shared_ptr<Object> object;shared_ptr<ArrayValue> array;shared_ptr<Bound> bound;};
    Value():storage{}{}
    void destroy(){switch(tag){case String:case Char:s.~Text();break;case Obj:object.~shared_ptr();break;case Arr:array.~shared_ptr();break;case Callable:bound.~shared_ptr();break;default:break;}tag=Void;}
    ~Value(){destroy();}
    Value(const Value& v):tag(v.tag){switch(tag){case String:case Char:new(&s)uma_native::Text(v.s);break;case Obj:new(&object)shared_ptr<Object>(v.object);break;case Arr:new(&array)shared_ptr<ArrayValue>(v.array);break;case Callable:new(&bound)shared_ptr<Bound>(v.bound);break;default:memcpy(&i,&v.i,8);}}
    Value(Value&& v)noexcept:tag(v.tag){switch(tag){case String:case Char:new(&s)uma_native::Text(move(v.s));break;case Obj:new(&object)shared_ptr<Object>(move(v.object));break;case Arr:new(&array)shared_ptr<ArrayValue>(move(v.array));break;case Callable:new(&bound)shared_ptr<Bound>(move(v.bound));break;default:memcpy(&i,&v.i,8);}v.destroy();}
    Value& operator=(const Value& v){if(this!=&v){destroy();new(this)Value(v);}return *this;}
    Value& operator=(Value&& v)noexcept{if(this!=&v){destroy();new(this)Value(move(v));}return *this;}
    static Value integer(int64_t x){Value v;v.tag=Int;v.i=x;return v;}
    static Value real(double x){Value v;v.tag=Double;v.d=x;return v;}
    static Value text(uma_native::Text x,bool character=false){Value v;v.tag=character?Char:String;new(&v.s)uma_native::Text(move(x));return v;}
    static Value boolean(bool x){Value v;v.tag=Bool;v.b=x;return v;}
    static Value null(){Value v;v.tag=Null;return v;}
    static Value instance(shared_ptr<Object> x){Value v;v.tag=Obj;new(&v.object)shared_ptr<Object>(move(x));return v;}
    static Value list(shared_ptr<ArrayValue> x){Value v;v.tag=Arr;new(&v.array)shared_ptr<ArrayValue>(move(x));return v;}
    static Value function(F f,shared_ptr<Object> o={},string n="");
};
static_assert(sizeof(Value)<=24,"Value deve manter representacao compacta");
struct Slot { string type; Value value; };
struct Object { string cls; unordered_map<string, Slot> fields; };
struct ArrayValue { string elem; vector<Value> values; };
struct Bound { F function; shared_ptr<Object> self; string native;vector<Value> captures; };
Value Value::function(F f, shared_ptr<Object> o, string n) { Value v; v.tag = Callable; new(&v.bound)shared_ptr<Bound>(make_shared<Bound>(Bound{move(f), move(o), move(n),{}})); return v; }
struct Environment { shared_ptr<Environment> parent; unordered_map<string, Slot> vars; explicit Environment(shared_ptr<Environment> p = {}) : parent(move(p)) {} };
struct Returned { Value value; }; struct Broken {}; struct Continued {};
struct LValue { string type; function<Value()> read; function<void(Value)> write; };
volatile sig_atomic_t interrupted = 0;
void interrupt(int) { interrupted = 1; }

struct Runtime {
    Program& p; shared_ptr<Environment> global = make_shared<Environment>(), env = global; shared_ptr<Object> self; string owner;
    uint64_t steps = 0, maxSteps = 0; size_t depth = 0, maxDepth = 1024;
    vector<weak_ptr<Object>> objects; vector<weak_ptr<ArrayValue>> arrays;
    size_t nextCollection=1024;
    void collect(){
        vector<shared_ptr<Object>> os;vector<shared_ptr<ArrayValue>> as;
        for(auto& w:objects)if(auto o=w.lock())os.push_back(move(o));
        for(auto& w:arrays)if(auto a=w.lock())as.push_back(move(a));
        vector<shared_ptr<Bound>> bs;unordered_set<Bound*> seen;
        auto discover=[&](const Value& v){if(v.tag==Value::Callable && v.bound && seen.insert(v.bound.get()).second)bs.push_back(v.bound);};
        for(auto& o:os)for(auto& f:o->fields)discover(f.second.value);
        for(auto& a:as)for(auto& v:a->values)discover(v);
        for(size_t i=0;i<bs.size();++i)for(auto& v:bs[i]->captures)discover(v);
        unordered_map<const void*,size_t> ids;
        for(size_t i=0;i<os.size();++i)ids[os[i].get()]=i;
        for(size_t i=0;i<as.size();++i)ids[as[i].get()]=os.size()+i;
        for(size_t i=0;i<bs.size();++i)ids[bs[i].get()]=os.size()+as.size()+i;
        vector<vector<size_t>> edges(ids.size());vector<size_t> incoming(ids.size()),work;vector<bool> live(ids.size());
        auto pointer=[&](size_t i,const void* ptr){auto it=ids.find(ptr);if(it!=ids.end()){edges[i].push_back(it->second);++incoming[it->second];}};
        auto edge=[&](size_t i,const Value& v){pointer(i,v.tag==Value::Obj?static_cast<const void*>(v.object.get()):v.tag==Value::Arr?static_cast<const void*>(v.array.get()):v.tag==Value::Callable?static_cast<const void*>(v.bound.get()):nullptr);};
        for(size_t i=0;i<os.size();++i)for(auto& f:os[i]->fields)edge(i,f.second.value);
        for(size_t i=0;i<as.size();++i)for(auto& v:as[i]->values)edge(os.size()+i,v);
        for(size_t i=0;i<bs.size();++i){auto n=os.size()+as.size()+i;pointer(n,bs[i]->self.get());for(auto& v:bs[i]->captures)edge(n,v);}
        for(size_t i=0;i<ids.size();++i){auto refs=i<os.size()?os[i].use_count():i<os.size()+as.size()?as[i-os.size()].use_count():bs[i-os.size()-as.size()].use_count();if(refs>static_cast<long>(incoming[i]+1)){live[i]=true;work.push_back(i);}}
        while(!work.empty()){auto i=work.back();work.pop_back();for(auto j:edges[i])if(!live[j]){live[j]=true;work.push_back(j);}}
        for(size_t i=0;i<os.size();++i)if(!live[i])os[i]->fields.clear();
        for(size_t i=0;i<as.size();++i)if(!live[os.size()+i])as[i]->values.clear();
        for(size_t i=0;i<bs.size();++i)if(!live[os.size()+as.size()+i]){bs[i]->self.reset();bs[i]->captures.clear();}
        bs.clear();
        os.clear();as.clear();
        objects.erase(remove_if(objects.begin(),objects.end(),[](auto& w){return w.expired();}),objects.end());
        arrays.erase(remove_if(arrays.begin(),arrays.end(),[](auto& w){return w.expired();}),arrays.end());
        nextCollection=max<size_t>(1024,(objects.size()+arrays.size())*2+1);
    }
    void allocation(){if(objects.size()+arrays.size()>=nextCollection)collect();}
    explicit Runtime(Program& x) : p(x) {}
    ~Runtime() {
        // Libera tambem ciclos de objetos ao encerrar a execucao.
        vector<shared_ptr<Object>> os; vector<shared_ptr<ArrayValue>> as;
        for (auto& w : objects) if (auto o = w.lock()) os.push_back(o);
        for (auto& w : arrays) if (auto a = w.lock()) as.push_back(a);
        for (auto& o : os) o->fields.clear();
        for (auto& a : as) a->values.clear();
    }
    struct Context {
        Runtime& r; shared_ptr<Environment> env; shared_ptr<Object> self; string owner;
        explicit Context(Runtime& r) : r(r), env(r.env), self(r.self), owner(r.owner) {}
        ~Context() { r.env = env; r.self = self; r.owner = owner; }
    };
    void tick(const Token& t) {
        if (interrupted) fail(t, "interrupcao", "Execucao interrompida.");
        if (maxSteps && ++steps > maxSteps) fail(t, "execucao", "Limite de passos atingido (--max-steps).");
    }
    Value coerce(Value v, const string& type) {
        if (type == "stamina" && v.tag == Value::Int) return Value::real(static_cast<double>(v.i));
        return v;
    }
    shared_ptr<ArrayValue> array(const string& elem) { allocation();auto a = make_shared<ArrayValue>(); a->elem = elem; arrays.push_back(a); return a; }
    Value defaultValue(const string& t) {
        if (t == "speed") return Value::integer(0);
        if (t == "stamina") return Value::real(0);
        if (t == "wit") return Value::text("");
        if (t == "guts") return Value::text(string(1, '\0'), true);
        if (t == "track") return Value::boolean(false);
        if (arrayType(t)) return Value::list(array(elementType(t)));
        return Value::null();
    }
    string format(const Value& v, int nesting = 0) {
        switch (v.tag) {
        case Value::Void: return "power"; case Value::Int: return to_string(v.i);
        case Value::Double: { ostringstream out; out << setprecision(17) << v.d; return out.str(); }
        case Value::String: case Value::Char: return v.s; case Value::Bool: return v.b ? "turf" : "dirt";
        case Value::Null: return "nature"; case Value::Obj: return "<" + v.object->cls + ">";
        case Value::Arr: { if (nesting >= 32) return "[...]"; string out = "["; for (size_t i = 0; i < v.array->values.size(); ++i) { if (i) out += ", "; out += format(v.array->values[i], nesting + 1); } return out + "]"; }
        case Value::Callable: return "<funcao>";
        }
        return "";
    }
    double real(const Value& v) { return v.tag == Value::Int ? static_cast<double>(v.i) : v.d; }
    Value finite(double d, const Token& t) { if (!isfinite(d)) fail(t, "execucao", "Resultado stamina nao finito."); return Value::real(d); }
    Value operation(const Value& a, const Value& b, const Token& t, const string& s) {
        if(s=="&" || s=="|" || s=="^" || s=="<<" || s==">>") {
            try { return Value::integer(uma_native::bitop(a.i,b.i,s.c_str(),{t.file.c_str(),t.line,t.col})); }
            catch(const uma_native::Failure& e){fail(t,"execucao",e.what());}
        }
        if(s=="+%" || s=="-%" || s=="*%")return Value::integer(s=="+%"?uma_native::wrapAdd(a.i,b.i):s=="-%"?uma_native::wrapSub(a.i,b.i):uma_native::wrapMul(a.i,b.i));
        bool num = (a.tag == Value::Int || a.tag == Value::Double) && (b.tag == Value::Int || b.tag == Value::Double);
        if (s == "==" || s == "!=") {
            bool eq = false;
            if (num) {
                // Evita arredondar dois inteiros grandes para o mesmo double.
                if (a.tag == Value::Int && b.tag == Value::Int) eq = a.i == b.i;
                else if (a.tag == Value::Double && b.tag == Value::Double) eq = a.d == b.d;
                else { int64_t i = a.tag == Value::Int ? a.i : b.i; double d = a.tag == Value::Double ? a.d : b.d; eq = d >= -9223372036854775808.0 && d < 9223372036854775808.0 && trunc(d) == d && static_cast<int64_t>(d) == i; }
            } else if (a.tag == b.tag) {
                if (a.tag == Value::Null) eq = true; else if (a.tag == Value::Bool) eq = a.b == b.b;
                else if (a.tag == Value::String || a.tag == Value::Char) eq = a.s == b.s;
                else if (a.tag == Value::Obj) eq = a.object == b.object; else if (a.tag == Value::Arr) eq = a.array == b.array;
            }
            return Value::boolean(s == "==" ? eq : !eq);
        }
        if (s == "<" || s == ">" || s == "<=" || s == ">=") {
            int cmp = 0;
            if (!num) cmp = a.s.compare(b.s);
            else if (a.tag == Value::Int && b.tag == Value::Int) cmp = a.i < b.i ? -1 : a.i > b.i ? 1 : 0;
            else if (a.tag == Value::Double && b.tag == Value::Double) cmp = a.d < b.d ? -1 : a.d > b.d ? 1 : 0;
            else {
                int64_t i = a.tag == Value::Int ? a.i : b.i; double d = a.tag == Value::Double ? a.d : b.d; int order;
                if (d < -9223372036854775808.0) order = 1;
                else if (d >= 9223372036854775808.0) order = -1;
                else { int64_t integral = static_cast<int64_t>(d); order = i < integral ? -1 : i > integral ? 1 : (d > trunc(d) ? -1 : d < trunc(d) ? 1 : 0); }
                cmp = a.tag == Value::Int ? order : -order;
            }
            return Value::boolean(s == "<" ? cmp < 0 : s == ">" ? cmp > 0 : s == "<=" ? cmp <= 0 : cmp >= 0);
        }
        if (s == "+" && !num) return Value::text(a.s + b.s);
        if (a.tag == Value::Double || b.tag == Value::Double) {
            double x = real(a), y = real(b);
            if (s == "/" && y == 0) fail(t, "execucao", "Divisao por zero.");
            return finite(s == "+" ? x + y : s == "-" ? x - y : s == "*" ? x * y : x / y, t);
        }
        int64_t x = a.i, y = b.i, v = 0; const int64_t lo = numeric_limits<int64_t>::min(), hi = numeric_limits<int64_t>::max(); bool overflow = false;
        if (s == "+") { overflow = y > 0 ? x > hi - y : y < 0 && x < lo - y; if (!overflow) v = x + y; }
        else if (s == "-") { overflow = y < 0 ? x > hi + y : y > 0 && x < lo + y; if (!overflow) v = x - y; }
        else if (s == "*") {
            if (x > 0) overflow = y > 0 ? x > hi / y : y < lo / x;
            else if (x < 0) overflow = y > 0 ? x < lo / y : y < 0 && x < hi / y;
            if (!overflow) v = x * y;
        } else {
            if (y == 0) fail(t, "execucao", "Divisao por zero.");
            overflow = x == lo && y == -1;
            if (!overflow) v = s == "%" ? x % y : x / y;
            else if (s == "%") { overflow = false; v = 0; }
        }
        if (overflow) fail(t, "execucao", "Overflow de speed (inteiro de 64 bits).");
        return Value::integer(v);
    }
    size_t index(const Value& v, size_t size, const Token& t) {
        if (v.i < 0 || static_cast<uint64_t>(v.i) >= static_cast<uint64_t>(size)) fail(t, "execucao", "Indice fora dos limites: " + to_string(v.i) + ".");
        return static_cast<size_t>(v.i);
    }
    Value native(const string& n, vector<Value> vs, const Token& t) {
        if(standardNatives.count(n)) {
            uma_native::Loc l{t.file.c_str(),t.line,t.col};
            try {

                if(n=="uma_json_normalize")return Value::text(uma_native::jsonNormalize(vs[0].s,l));
                if(n=="uma_json_get")return Value::text(uma_native::jsonGet(vs[0].s,vs[1].s,l));
                if(n=="uma_json_at")return Value::text(uma_native::jsonAt(vs[0].s,vs[1].i,l));
                if(n=="uma_json_kind")return Value::text(uma_native::jsonKind(vs[0].s,l));
                if(n=="uma_json_value")return Value::text(uma_native::jsonValue(vs[0].s,l));
                if(n=="uma_json_size")return Value::integer(uma_native::jsonSize(vs[0].s,l));
                if(n=="uma_json_set")return Value::text(uma_native::jsonSet(vs[0].s,vs[1].s,vs[2].s,l));
                if(n=="uma_json_quote")return Value::text(uma_native::jsonQuote(vs[0].s));
                if(n=="uma_base64_encode"){if(vs[0].tag==Value::Null)fail(t,"execucao","Base64 recebeu nature.");string bytes;for(auto& v:vs[0].array->values){if(v.i<0 || v.i>255)fail(t,"execucao","Byte fora do intervalo.");bytes+=char(v.i);}return Value::text(uma_native::base64Encode(bytes));}
                if(n=="uma_base64_decode"){auto a=array("speed");for(unsigned char c:uma_native::base64Decode(vs[0].s,l))a->values.push_back(Value::integer(c));return Value::list(a);}
                if(n=="uma_args" || n=="uma_list_dir"){auto a=array("wit");auto list=n=="uma_args"?uma_native::arguments:uma_native::listDir(vs[0].s,l);for(auto& s:list)a->values.push_back(Value::text(move(s)));return Value::list(a);}
                if(n=="uma_eof")return Value::boolean(uma_native::inputEnd());
                if(n=="uma_read_all")return Value::text(uma_native::readAll(l));
                if(n=="uma_exit")uma_native::exitProgram(vs[0].i,l);
                if(n=="uma_stderr"){cerr<<vs[0].s.str()<<'\n';return {};}
                if(n=="uma_hash")return Value::integer(uma_native::hashText(vs[0].s));
                if(n=="uma_replace")return Value::text(uma_native::replaceText(vs[0].s,vs[1].s,vs[2].s,l));
                if(n=="uma_lower" || n=="uma_upper")return Value::text(uma_native::asciiCase(vs[0].s,n=="uma_upper"));
                if(n=="uma_regex_replace")return Value::text(uma_native::regexReplace(vs[0].s,vs[1].s,vs[2].s,l));
                if(n=="uma_mkdir"){uma_native::mkdir(vs[0].s,l);return {};}
                if(n=="uma_remove")return Value::boolean(uma_native::removePath(vs[0].s,l));
                if(n=="uma_path_join")return Value::text(uma_native::pathJoin(vs[0].s,vs[1].s,l));
                if(n=="uma_basename")return Value::text(filesystem::u8path(uma_native::pathText(vs[0].s,l)).filename().u8string());
                if(n=="uma_random")return Value::integer(uma_native::randomInt());
                if(n=="uma_unix_time")return Value::integer(uma_native::unixTime());
                if(n=="uma_collect"){collect();return {};}
                if(n=="uma_clock")return Value::integer(uma_native::clock());
                if(n=="uma_sleep"){uma_native::sleep(vs[0].i,l);return {};}
                if(n=="uma_read_text")return Value::text(uma_native::read(vs[0].s,l));
                if(n=="uma_write_text" || n=="uma_append_text"){uma_native::write(vs[0].s,vs[1].s,l,n=="uma_append_text");return {};}
                if(n=="uma_exists")return Value::boolean(uma_native::exists(vs[0].s,l));
                if(n=="uma_env")return Value::text(uma_native::env(vs[0].s,l));
                if(n=="uma_slice")return Value::text(uma_native::slice(vs[0].s,vs[1].i,vs[2].i,l));
                if(n=="uma_find")return Value::integer(uma_native::find(vs[0].s,vs[1].s,l));
                if(n=="uma_read_bytes"){auto a=array("speed");for(unsigned char c:uma_native::read(vs[0].s,l,false))a->values.push_back(Value::integer(c));return Value::list(a);}
                if(n=="uma_write_bytes"){
                    if(vs[1].tag==Value::Null)fail(t,"execucao","Array de bytes recebeu nature.");
                    string bytes;for(const auto& v:vs[1].array->values){if(v.i<0 || v.i>255)fail(t,"execucao","Byte fora do intervalo 0..255.");bytes+=static_cast<char>(v.i);}uma_native::write(vs[0].s,bytes,l);return {};
                }
                double a=real(vs[0]), out=0;
                if(n=="uma_sqrt")out=sqrt(a);else if(n=="uma_pow")out=pow(a,real(vs[1]));else if(n=="uma_sin")out=sin(a);else if(n=="uma_cos")out=cos(a);else if(n=="uma_floor")out=floor(a);else if(n=="uma_ceil")out=ceil(a);else if(n=="uma_log")out=log(a);
                return finite(out,t);
            } catch(const uma_native::Failure& e){fail(t,"execucao",e.what());}
        }
        if(n=="ordenar"){if(vs[0].tag==Value::Null)fail(t,"execucao","ordenar recebeu nature.");auto& a=vs[0].array->values;sort(a.begin(),a.end(),[&](const Value& x,const Value& y){return operation(x,y,t,"<").b;});return {};}
        if (n == "bakushin") { for (size_t i = 0; i < vs.size(); ++i) { if (i) cout << ' '; cout << format(vs[i]); } cout << '\n'; return {}; }
        if (n == "suzuka") { string line; if (!getline(cin, line)) fail(t, "execucao", "suzuka encontrou fim da entrada."); if (!line.empty() && line.back() == '\r') line.pop_back(); boundaries(line, t); return Value::text(line); }
        if (n == "flash") { if (!vs[0].b) fail(t, "execucao", vs.size() == 2 ? "flash: " + vs[1].s : "flash: verificacao falhou."); return {}; }
        Value a = vs[0];
        if (n == "maruzensky") return Value::text(format(a));
        if (n == "oguri") {
            if (a.tag == Value::Null) fail(t, "execucao", "oguri recebeu nature.");
            size_t len = a.tag == Value::String ? a.s.length({t.file.c_str(),t.line,t.col}) : a.array->values.size();
            if (len > static_cast<uint64_t>(numeric_limits<int64_t>::max())) fail(t, "execucao", "Comprimento nao cabe em speed.");
            return Value::integer(static_cast<int64_t>(len));
        }
        if (n == "falcon" || n == "dober") {
            if (a.tag == Value::Null) fail(t, "execucao", n + " recebeu nature.");
            if (n == "falcon") { a.array->values.push_back(coerce(vs[1], a.array->elem)); return {}; }
            if (a.array->values.empty()) fail(t, "execucao", "dober nao pode retirar de array vazio.");
            Value v = a.array->values.back(); a.array->values.pop_back(); return v;
        }
        if (n == "hayahide") {
            uint32_t cp; unsigned char c = static_cast<unsigned char>(a.s[0]);
            if (c < 128) cp = c; else { size_t k = c < 224 ? 2 : c < 240 ? 3 : 4; cp = c & (k == 2 ? 31u : k == 3 ? 15u : 7u); for (size_t i = 1; i < k; ++i) cp = (cp << 6) | (static_cast<unsigned char>(a.s[i]) & 63); }
            return Value::integer(cp);
        }
        if (n == "ticket") { if (a.i < 0 || a.i > 0x10ffff || !scalar(static_cast<uint32_t>(a.i))) fail(t, "execucao", "ticket exige um codigo Unicode valido."); return Value::text(encode(static_cast<uint32_t>(a.i)), true); }
        if (n == "taiki") {
            if (a.tag == Value::Int) return a;
            if (a.tag == Value::Double) { if (a.d < -9223372036854775808.0 || a.d >= 9223372036854775808.0) fail(t, "execucao", "Conversao para speed fora do intervalo."); return Value::integer(static_cast<int64_t>(a.d)); }
            try { size_t used; int64_t i = stoll(a.s, &used, 10); if (used != a.s.size()) throw invalid_argument("suffix"); return Value::integer(i); } catch (const exception&) { fail(t, "execucao", "taiki: texto nao representa um speed valido."); }
        }
        if (a.tag == Value::Int || a.tag == Value::Double) return finite(real(a), t);
        try { size_t used; double d = stod(a.s, &used); if (used != a.s.size() || !isfinite(d)) throw invalid_argument("number"); return Value::real(d); } catch (const exception&) { fail(t, "execucao", "creek: texto nao representa um stamina valido."); }
    }

};
void diagnostic(const Error& e, const string& path, const string& source) {
    cerr << path << ':' << e.at.line << ':' << e.at.col << ": erro de " << e.phase << " ["<<e.code()<<"]: " << e.what() << '\n';
    istringstream in(source); string line; for (int i = 1; i <= e.at.line && getline(in, line); ++i) if (i == e.at.line) {
        if (line.size() <= 300) { cerr << "  " << line << '\n' << "  "; for (int j = 1; j < e.at.col; ++j) cerr << (size_t(j - 1) < line.size() && line[size_t(j - 1)] == '\t' ? '\t' : ' '); cerr << string(min<size_t>(48,max<size_t>(1,e.at.end-e.at.begin)),'^') << "\n"; }
    }
    for (auto& s : e.stack) cerr << "  em " << s << '\n';
}
} // namespace uma
