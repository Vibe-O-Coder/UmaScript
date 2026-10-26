#pragma once
#include "ownership.hpp"
namespace uma {
string formatSource(const string& source,const string& dialect){
    if(!dialect.empty() && dialect!="uma" && dialect!="classico")throw invalid_argument("Dialeto deve ser uma ou classico.");
    auto tokens=Lexer(source).run();string rewritten;size_t cursor=0;
    for(auto& t:tokens){if(t.kind=="end")break;rewritten+=source.substr(cursor,t.begin-cursor);string text=source.substr(t.begin,t.end-t.begin);if(t.kind=="id" && !dialect.empty()){if(dialect=="classico"){auto i=vocabularyClassic.find(t.text);if(i!=vocabularyClassic.end())text=i->second;}else if(vocabularyClassic.count(t.text))text=t.text;}rewritten+=text;cursor=t.end;}
    rewritten+=source.substr(cursor);auto formattedTokens=Lexer(rewritten).run();map<int,pair<int,int>> braces;
    for(auto& t:formattedTokens){if(t.kind=="op" && t.text=="{")++braces[t.line].first;if(t.kind=="op" && t.text=="}")++braces[t.line].second;}
    istringstream in(rewritten);string line,out;int indent=0,n=0;bool blockComment=false;
    while(getline(in,line)){++n;if(!line.empty() && line.back()=='\r')line.pop_back();auto first=line.find_first_not_of(" \t");auto last=line.find_last_not_of(" \t");if(first==string::npos){out+='\n';continue;}
        auto text=line.substr(first,last-first+1);int closing=text[0]=='}'?1:0;
        if(blockComment)out+=line+"\n";else out+=string(static_cast<size_t>(max(0,indent-closing))*4,' ')+text+"\n";
        if(text.find("/*")!=string::npos && text.find("*/")==string::npos)blockComment=true;
        if(text.find("*/")!=string::npos)blockComment=false;
        indent=max(0,indent+braces[n].first-braces[n].second);
    }
    return out;
}
int repl(const filesystem::path& root){
    cerr<<"UmaScript "<<umaVersion<<". Digite :sair para encerrar; blocos podem ocupar varias linhas.\n";
    string accepted,pending,line;Module module;VM vm(module);size_t start=0;
    while(cerr<<(pending.empty()?"> ":"... "),getline(cin,line)){
        if(line==":sair" || line==":quit")break;
        pending+=line+'\n';
        int braces=0;
        try{for(auto& t:Lexer(pending).run()){if(t.kind=="op" && t.text=="{")++braces;if(t.kind=="op" && t.text=="}")--braces;}if(braces>0)continue;
            SourceLoader loader(root);loader.fromStdin=true;loader.inputText=accepted+pending;auto tokens=loader.load(filesystem::current_path()/"repl.uma",true);auto p=Parser(move(tokens)).run();Checker(p).run();OwnershipChecker(p).run();auto next=compile(move(p),"repl.uma");verify(next);module=move(next);auto begin=start;start=module.codes.at(module.entry).size()-2;accepted+=pending;interrupted=0;vm.execute(module.entry,{},begin);
        }catch(const Error& e){diagnostic(e,"repl.uma",accepted+pending);}catch(const uma_native::Exit& e){return e.code;}catch(const exception& e){cerr<<e.what()<<'\n';}
        pending.clear();
    }
    return 0;
}
}
