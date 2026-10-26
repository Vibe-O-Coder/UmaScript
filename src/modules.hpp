#pragma once
#include "language.hpp"
namespace uma {
struct SourceLoader {
    filesystem::path root;
    map<string,string> sources;
    map<string,string> overlays;
    set<string> loaded,active;
    size_t bytes=0;
    bool fromStdin=false;string inputText;
    map<string,pair<string,set<string>>> namespaces;
    explicit SourceLoader(filesystem::path root):root(move(root)){}
    vector<Token> load(const filesystem::path& file,bool entry=false) {
        auto canonical=filesystem::weakly_canonical(filesystem::absolute(file));string path=canonical.u8string();
        Token at{"import","id",1,1,path};
        if(active.count(path))fail(at,"modulo","Importacao ciclica: "+path);
        if(loaded.count(path))return {};
        if(active.size()>=64 || loaded.size()>=512)fail(at,"modulo","Limite de modulos excedido (64 niveis, 512 arquivos).");
        string source;
        if(overlays.count(path))source=overlays.at(path);
        else if(entry && fromStdin)source=inputText;
        else {ifstream in(canonical,ios::binary);if(!in)fail(at,"modulo","Nao foi possivel abrir modulo: "+path);in.seekg(0,ios::end);auto len=in.tellg();if(len<0 || static_cast<uint64_t>(len)>16u*1024u*1024u)fail(at,"modulo","Modulo excede 16 MiB.");source.resize(static_cast<size_t>(len));in.seekg(0);if(!source.empty() && !in.read(source.data(),static_cast<streamsize>(source.size())))fail(at,"modulo","Falha ao ler modulo.");}
        bytes+=source.size();if(bytes>16u*1024u*1024u)fail(at,"modulo","Projeto excede 16 MiB de codigo-fonte.");
        sources[path]=source;active.insert(path);
        vector<Token> ts;
        try {ts=Lexer(source).run();}catch(Error& e){e.at.file=path;throw;}
        for(auto& t:ts)t.file=path;
        vector<Token> out;int braces=0;map<string,pair<string,set<string>>> aliases;
        for(size_t i=0;i+1<ts.size();++i){
            auto t=ts[i];
            if(t.kind=="id" && (t.text=="import" || t.text=="convocar")){
                if(braces)fail(t,"modulo","import so pode aparecer no escopo global.");
                bool aliased=i+4<ts.size() && ts[i+2].text=="como" && ts[i+3].kind=="id" && ts[i+4].text==";";
                if(i+2>=ts.size() || ts[i+1].kind!="string" || (!aliased && ts[i+2].text!=";"))fail(t,"modulo","Use convocar \"caminho.uma\" como nome; ou import \"caminho.uma\";");
                string name=ts[i+1].text;if(name.empty() || name.find('\0')!=string::npos)fail(t,"modulo","Caminho de importacao invalido.");
                filesystem::path target;
                if(name.rfind("std/",0)==0){auto relative=filesystem::u8path(name.substr(4));for(const auto& part:relative)if(part=="..")fail(t,"modulo","Modulo padrao nao pode escapar de stdlib.");target=root/"stdlib"/relative;}
                else target=canonical.parent_path()/filesystem::u8path(name);
                if(!target.has_extension())target+=".uma";
                if(aliased){
                    string alias=ts[i+3].text;if(reserved(alias) || aliases.count(alias))fail(ts[i+3],"modulo","Alias duplicado ou reservado: "+alias);
                    string key=filesystem::weakly_canonical(target).u8string();auto found=namespaces.find(key);
                    if(found==namespaces.end()){
                        SourceLoader child(root);child.active=active;child.overlays=overlays;child.namespaces=namespaces;
                        auto more=child.load(target,true);auto parsed=Parser(more).run();
                        set<string> all,exports,hidden;set<tuple<string,int,int>> memberTokens;
                        for(auto& kv:parsed.functions){all.insert(kv.first);if(kv.second->token.file==key && !kv.second->priv)exports.insert(kv.first);}
                        for(auto& kv:parsed.interfaces){all.insert(kv.first);if(kv.second->token.file==key)exports.insert(kv.first);for(auto& f:kv.second->methods)memberTokens.insert({f.second->token.file,f.second->token.line,f.second->token.col});}
                        for(auto& kv:parsed.classes){all.insert(kv.first);if(kv.second->token.file==key)exports.insert(kv.first);for(auto& f:kv.second->fields)memberTokens.insert({f.token.file,f.token.line,f.token.col});for(auto& f:kv.second->methods)memberTokens.insert({f.second->token.file,f.second->token.line,f.second->token.col});}
                        for(auto& kv:parsed.templates){all.insert(kv.first);exports.insert(kv.first);}
                        for(auto& s:parsed.statements)if(s->kind==Stmt::Variable){all.insert(s->name);if(s->token.file==key)exports.insert(s->name);}
                        string prefix="_module_"+to_string(namespaces.size())+"_";
                        for(size_t j=0;j+1<more.size();++j){auto token=more[j];bool memberToken=memberTokens.count({token.file,token.line,token.col});if(token.kind=="id" && all.count(token.text) && !memberToken && !(j && more[j-1].text=="."))token.text=prefix+token.text;out.push_back(move(token));}
                        sources.insert(child.sources.begin(),child.sources.end());bytes+=child.bytes;if(bytes>16u*1024u*1024u)fail(t,"modulo","Projeto excede 16 MiB de codigo-fonte.");
                        namespaces[key]={prefix,exports};
                        found=namespaces.find(key);
                    }
                    aliases[alias]=found->second;i+=4;continue;
                }
                try{auto more=load(target);out.insert(out.end(),more.begin(),more.end());}catch(Error& e){if(e.at.line==1 && e.at.col==1 && e.phase=="modulo")e.stack.push_back("importado por "+path+":"+to_string(t.line));throw;}
                i+=2;continue;
            }
            if(t.kind=="id" && aliases.count(t.text) && i+2<ts.size() && ts[i+1].text=="."){
                auto ns=aliases.at(t.text);auto name=ts[i+2];if(!ns.second.count(name.text))fail(name,"modulo","Nome nao exportado pelo modulo: "+name.text);name.text=ns.first+name.text;out.push_back(move(name));i+=2;continue;
            }
            if(t.kind=="op" && t.text=="{")++braces;
            if(t.kind=="op" && t.text=="}")--braces;
            out.push_back(move(t));
        }
        active.erase(path);loaded.insert(path);if(entry)out.push_back(ts.back());return out;
    }
};
}
