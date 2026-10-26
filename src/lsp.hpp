#pragma once
#include "tooling.hpp"
#include "initialization.hpp"
namespace uma {
struct LanguageServer {
    filesystem::path root;map<string,string> documents;bool shutdown=false;
    explicit LanguageServer(filesystem::path root):root(move(root)){}
    using J=uma_native::Json;
    static const J& get(const J& j,const string& key){static J empty;auto it=j.object.find(key);return it==j.object.end()?empty:it->second;}
    static int number(const J& j){try{return stoi(j.text);}catch(...){return 0;}}
    static string decodeUri(string uri){if(uri.rfind("file://",0)!=0)throw invalid_argument("LSP aceita URIs file://.");uri.erase(0,7);string path;for(size_t i=0;i<uri.size();++i){if(uri[i]=='%' && i+2<uri.size()){path+=char(stoul(uri.substr(i+1,2),nullptr,16));i+=2;}else path+=uri[i];}
#ifdef _WIN32
        if(path.size()>2 && path[0]=='/' && path[2]==':')path.erase(0,1);
#endif
        return path;
    }
    static string uri(const string& path){string s=filesystem::absolute(filesystem::u8path(path)).generic_u8string(),out="file://";
#ifdef _WIN32
        out+='/';
#endif
        const char* hex="0123456789ABCDEF";for(unsigned char c:s){if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || string("/-_.:").find(char(c))!=string::npos)out+=char(c);else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;
    }
    static size_t byteOffset(const string& s,int line,int character){size_t p=0;while(line-->0 && p<s.size()){auto next=s.find('\n',p);p=next==string::npos?s.size():next+1;}int units=0;while(p<s.size() && s[p]!='\n' && units<character){auto c=static_cast<unsigned char>(s[p]);size_t bytes=c<128?1:c<224?2:c<240?3:4;units+=bytes==4?2:1;p+=min(bytes,s.size()-p);}return p;}
    static int utf16Column(const string& s,int line,int column){auto start=byteOffset(s,line,0);size_t end=min(s.size(),start+static_cast<size_t>(max(0,column)));int out=0;for(auto p=start;p<end;){auto c=static_cast<unsigned char>(s[p]);auto n=c<128?1:c<224?2:c<240?3:4;out+=n==4?2:1;p+=static_cast<size_t>(n);}return out;}
    static string position(int line,int col){return "{\"line\":"+to_string(line)+",\"character\":"+to_string(col)+"}";}
    static string range(int line,int col,int width=1){return "{\"start\":"+position(line,col)+",\"end\":"+position(line,col+width)+"}";}
    static void send(const string& value){cout<<"Content-Length: "<<value.size()<<"\r\n\r\n"<<value<<flush;}
    void notify(const string& method,const string& params){send("{\"jsonrpc\":\"2.0\",\"method\":"+jsonString(method)+",\"params\":"+params+"}");}
    Program analyze(const string& document){SourceLoader loader(root);loader.fromStdin=true;loader.inputText=documents.at(document);for(auto& kv:documents)loader.overlays[filesystem::weakly_canonical(filesystem::absolute(filesystem::u8path(decodeUri(kv.first)))).u8string()]=kv.second;auto tokens=loader.load(filesystem::u8path(decodeUri(document)),true);auto p=Parser(move(tokens)).run();Checker(p).run();InitializationChecker(p).run();OwnershipChecker(p).run();return p;}
    void diagnostics(const string& document){
        string result="[]";try{analyze(document);}catch(const Error& e){int line=max(0,e.at.line-1),column=utf16Column(documents.at(document),line,max(0,e.at.col-1));result="[{\"range\":"+range(line,column)+",\"severity\":1,\"code\":"+jsonString(e.code())+",\"source\":\"UmaScript\",\"message\":"+jsonString(e.what())+"}]";}catch(const exception& e){result="[{\"range\":"+range(0,0)+",\"severity\":1,\"source\":\"UmaScript\",\"message\":"+jsonString(e.what())+"}]";}
        notify("textDocument/publishDiagnostics","{\"uri\":"+jsonString(document)+",\"diagnostics\":"+result+"}");
    }
    string request(const string& method,const J& params){
        if(method=="initialize")return "{\"capabilities\":{\"textDocumentSync\":1,\"documentFormattingProvider\":true,\"hoverProvider\":true,\"documentSymbolProvider\":true,\"completionProvider\":{\"resolveProvider\":false}},\"serverInfo\":{\"name\":\"UmaScript\",\"version\":"+jsonString(umaVersion)+"}}";
        if(method=="shutdown"){shutdown=true;return "null";}
        string document=get(get(params,"textDocument"),"uri").text;if(!documents.count(document))return "null";
        const auto& source=documents.at(document);
        if(method=="textDocument/formatting"){int line=static_cast<int>(count(source.begin(),source.end(),'\n'));auto last=source.find_last_of('\n');int col=utf16Column(source,line,static_cast<int>(source.size()-(last==string::npos?0:last+1)));return "[{\"range\":{\"start\":"+position(0,0)+",\"end\":"+position(line,col)+"},\"newText\":"+jsonString(formatSource(source,""))+"}]";}
        auto p=analyze(document);auto meta=uma_native::jsonParse(metadata(p),{"<lsp>",1,1});const auto& symbols=get(meta,"symbols").array;
        if(method=="textDocument/documentSymbol"){string out="[";for(auto& symbol:symbols){auto name=get(symbol,"name").text;if(name.rfind("_",0)==0 || uri(get(symbol,"file").text)!=document)continue;if(out.size()>1)out+=',';int line=number(get(symbol,"line"))-1,col=utf16Column(source,line,number(get(symbol,"column"))-1);out+="{\"name\":"+jsonString(name)+",\"kind\":13,\"range\":"+range(line,col,static_cast<int>(name.size()))+",\"selectionRange\":"+range(line,col,static_cast<int>(name.size()))+"}";}return out+"]";}
        if(method=="textDocument/completion"){string out="[";set<string> names;for(auto& symbol:symbols){auto name=get(symbol,"name").text;if(name.rfind("_",0)==0 || get(symbol,"private").text=="true" || !names.insert(name).second)continue;if(out.size()>1)out+=',';out+="{\"label\":"+jsonString(name)+",\"detail\":"+jsonString(get(symbol,"signature").text)+"}";}for(auto& kv:standardNatives)if(names.insert(kv.first).second){if(out.size()>1)out+=',';out+="{\"label\":"+jsonString(kv.first)+",\"detail\":"+jsonString(kv.second.documentation)+"}";}return out+"]";}
        if(method=="textDocument/hover"){auto pos=get(params,"position");auto offset=byteOffset(source,number(get(pos,"line")),number(get(pos,"character")));string word;for(auto& token:Lexer(source).run())if(token.begin<=offset && offset<token.end){word=token.text;break;}string text;auto kw=vocabularyClassic.find(word);if(kw!=vocabularyClassic.end())text=word+" = "+kw->second;auto native=standardNatives.find(word);if(native!=standardNatives.end())text=native->second.documentation;for(auto& symbol:symbols)if(get(symbol,"name").text==word){text=get(symbol,"signature").text;break;}if(text.empty())return "null";return "{\"contents\":{\"kind\":\"plaintext\",\"value\":"+jsonString(text)+"}}";}
        return "null";
    }
    int run(){
        string header;while(getline(cin,header)){size_t length=0;do{if(!header.empty() && header.back()=='\r')header.pop_back();if(header.rfind("Content-Length:",0)==0)length=stoull(header.substr(15));if(header.empty())break;}while(getline(cin,header));if(!length || length>16u*1024u*1024u)return 2;string bytes(length,'\0');if(!cin.read(bytes.data(),static_cast<streamsize>(length)))return 2;
            J message;try{message=uma_native::jsonParse(bytes,{"<lsp>",1,1});}catch(...){send("{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32700,\"message\":\"JSON invalido\"}}");continue;}
            string method=get(message,"method").text;auto params=get(message,"params");auto id=message.object.find("id");if(method=="exit")return shutdown?0:1;
            if(method=="textDocument/didOpen" || method=="textDocument/didChange"){auto doc=get(params,"textDocument");auto name=get(doc,"uri").text;if(method=="textDocument/didOpen")documents[name]=get(doc,"text").text;else{auto& changes=get(params,"contentChanges").array;if(!changes.empty())documents[name]=get(changes.back(),"text").text;}diagnostics(name);continue;}
            if(method=="textDocument/didClose"){auto name=get(get(params,"textDocument"),"uri").text;documents.erase(name);notify("textDocument/publishDiagnostics","{\"uri\":"+jsonString(name)+",\"diagnostics\":[]}");continue;}
            if(id==message.object.end())continue;
            auto prefix="{\"jsonrpc\":\"2.0\",\"id\":"+uma_native::jsonEncode(id->second);try{send(prefix+",\"result\":"+request(method,params)+"}");}catch(const exception& e){send(prefix+",\"error\":{\"code\":-32603,\"message\":"+jsonString(e.what())+"}}");}
        }return 0;
    }
};
}
