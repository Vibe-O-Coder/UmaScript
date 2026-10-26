#pragma once
#include "language.hpp"
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace uma {
int runProcess(const vector<string>& args,const filesystem::path& cwd={}){
    if(args.empty())throw invalid_argument("Processo sem executavel.");
#ifdef _WIN32
    auto wideProcess=[](const string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!n && !s.empty())throw invalid_argument("Argumento UTF-8 invalido.");wstring w(static_cast<size_t>(n),L'\0');if(n)MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;};
    wstring command;
    for(const auto& arg:args){if(!command.empty())command+=L' ';command+=L'"';auto w=wideProcess(arg);size_t slashes=0;for(wchar_t c:w){if(c==L'\\'){++slashes;continue;}if(c==L'"')command.append(slashes*2+1,L'\\');else command.append(slashes,L'\\');slashes=0;command+=c;}command.append(slashes*2,L'\\');command+=L'"';}
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    // As ferramentas MinGW precisam encontrar as DLLs de cc1plus no proprio bin.
    wchar_t* oldPath=nullptr;size_t oldSize=0;_wdupenv_s(&oldPath,&oldSize,L"PATH");wstring savedPath=oldPath?oldPath:L"";free(oldPath);
    auto parent=filesystem::u8path(args[0]).parent_path();
    struct RestorePath{wstring value;~RestorePath(){SetEnvironmentVariableW(L"PATH",value.c_str());}}restore{savedPath};
    if(!parent.empty())SetEnvironmentVariableW(L"PATH",(parent.wstring()+L";"+savedPath).c_str());
    if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,TRUE,0,nullptr,cwd.empty()?nullptr:cwd.c_str(),&startup,&process))throw runtime_error("Nao foi possivel iniciar o compilador C++/programa: "+args[0]);
    CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);return static_cast<int>(code);
#else
    pid_t pid=fork();if(pid<0)throw runtime_error("Falha ao criar processo.");
    if(!pid){if(!cwd.empty() && chdir(cwd.c_str())!=0)_exit(126);vector<char*> argv;for(const auto& a:args)argv.push_back(const_cast<char*>(a.c_str()));argv.push_back(nullptr);execvp(argv[0],argv.data());_exit(127);}
    int status=0;while(waitpid(pid,&status,0)<0){if(errno!=EINTR)throw runtime_error("Falha ao aguardar processo.");}return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
#endif
}
string findCxx(const string& requested){
    if(!requested.empty())return requested;
    if(const char* cc=getenv("UMA_CXX"))if(*cc)return cc;
    vector<string> names={"g++","clang++"};
#ifdef _WIN32
    names={"g++.exe","clang++.exe"};
#endif
    if(const char* value=getenv("PATH")){istringstream paths(value);string dir;
#ifdef _WIN32
        const char separator=';';
#else
        const char separator=':';
#endif
        while(getline(paths,dir,separator)){if(dir.size()>1 && dir.front()=='\"' && dir.back()=='\"')dir=dir.substr(1,dir.size()-2);for(const auto& name:names){auto p=filesystem::u8path(dir)/name;if(filesystem::is_regular_file(p))return p.u8string();}}
    }
#ifdef _WIN32
    for(const string& p:{string("C:/msys64/ucrt64/bin/g++.exe"),string("C:/msys64/mingw64/bin/g++.exe"),string("C:/Program Files/LLVM/bin/clang++.exe")})if(filesystem::is_regular_file(filesystem::u8path(p)))return p;
#endif
    throw invalid_argument("Compilacao nativa exige GCC ou Clang com C++17. Instale a ferramenta e use --cc CAMINHO ou UMA_CXX. O modo bytecode continua disponivel sem ela.");
}
}
