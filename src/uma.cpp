// UmaScript 2.0: compilador e runtime nativos, sem processos auxiliares.
#include "bytecode.hpp"
#include "native_backend.hpp"
#include "native_driver.hpp"
#include "modules.hpp"
#include "metadata.hpp"
#include "ownership.hpp"
#include "initialization.hpp"
#include "tooling.hpp"
#include "lsp.hpp"
#include <chrono>
#ifdef _WIN32
#include <shlobj.h>
#else
#include <unistd.h>
#endif

namespace uma {
namespace fs=std::filesystem;
const string version=umaVersion;
const string trailerMagic="UMARUN03";
#ifdef _WIN32
const string hostTarget="windows-x64", exeName="uma.exe";
#else
const string hostTarget="linux-x64", exeName="uma";
#endif
string utf8(const fs::path& p) {return p.u8string();}
fs::path pathOf(const string& s) {return fs::u8path(s);}
fs::path absPath(const fs::path& p) {return fs::absolute(p).lexically_normal();}
string readFile(const fs::path& p, uint64_t max=80u*1024u*1024u) {
    ifstream in(p,ios::binary);if(!in)throw invalid_argument("Nao foi possivel abrir: "+utf8(p));
    in.seekg(0,ios::end);auto len=in.tellg();if(len<0 || static_cast<uint64_t>(len)>max)throw invalid_argument("Arquivo excede o limite permitido: "+utf8(p));
    string s(static_cast<size_t>(len),'\0');in.seekg(0);if(!s.empty() && !in.read(s.data(),static_cast<streamsize>(s.size())))throw runtime_error("Falha ao ler: "+utf8(p));return s;
}
bool samePath(const fs::path& a,const fs::path& b) {
    error_code e;if(fs::equivalent(a,b,e))return true;return absPath(a)==absPath(b);
}
void writeFile(const fs::path& p,const string& bytes,bool executable=false) {
    if(fs::is_directory(p))throw invalid_argument("O destino e uma pasta: "+utf8(p));
    fs::path out=absPath(p), parent=out.parent_path();if(!fs::is_directory(parent))throw invalid_argument("Pasta de destino inexistente: "+utf8(parent));
    auto stamp=chrono::high_resolution_clock::now().time_since_epoch().count();
    fs::path temp=out;temp+=".uma-tmp-"+to_string(stamp);
    try {
        ofstream stream(temp,ios::binary|ios::trunc);if(!stream)throw runtime_error("Nao foi possivel gravar: "+utf8(p));
        stream.write(bytes.data(),static_cast<streamsize>(bytes.size()));stream.close();if(!stream)throw runtime_error("Falha na gravacao: "+utf8(p));
#ifdef _WIN32
        (void)executable;
        if(!MoveFileExW(temp.c_str(),out.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw runtime_error("Nao foi possivel substituir o destino: "+utf8(p));
#else
        if(executable)fs::permissions(temp,fs::perms::owner_all|fs::perms::group_read|fs::perms::group_exec|fs::perms::others_read|fs::perms::others_exec);
        else if(fs::exists(out))fs::permissions(temp,fs::status(out).permissions());
        fs::rename(temp,out);
#endif
    }catch(...){error_code e;fs::remove(temp,e);throw;}
}
fs::path ownExecutable(const string& fallback) {
#ifdef _WIN32
    (void)fallback;
    vector<wchar_t> buffer(32768);DWORD n=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));
    if(!n || n>=buffer.size())throw runtime_error("Nao foi possivel localizar uma.exe.");
    return fs::path(wstring(buffer.data(),n));
#else
    error_code e;auto p=fs::read_symlink("/proc/self/exe",e);if(!e)return p;
    return absPath(pathOf(fallback));
#endif
}
// Um executavel gerado e o runtime nativo seguido do bytecode e deste rodape.
// Nenhum arquivo temporario e executado e nenhum compilador externo e chamado.
bool extractPayload(const string& image,string& payload,size_t& imageSize) {
    imageSize=image.size();if(image.size()<16 || image.substr(image.size()-8)!=trailerMagic)return false;
    uint64_t n=0;for(int k=0;k<8;++k)n|=uint64_t(static_cast<unsigned char>(image[image.size()-16+size_t(k)]))<<(8*k);
    if(n>64u*1024u*1024u+4 || n>image.size()-16)throw invalid_argument("Executavel UmaScript corrompido.");
    imageSize=image.size()-16-static_cast<size_t>(n);payload=image.substr(imageSize,static_cast<size_t>(n));return true;
}
string buildImage(const fs::path& runtime,const string& bytecode,const string& target) {
    string image=readFile(runtime), old;size_t base=0;extractPayload(image,old,base);image.resize(base);
    bool pe=image.size()>64 && image.substr(0,2)=="MZ", elf=image.size()>64 && image.substr(0,4)==string("\x7f" "ELF",4);
    if((target=="windows-x64" && !pe) || (target=="linux-x64" && !elf))throw invalid_argument("Runtime incompativel com o alvo: "+target);
    image+=bytecode;Writer w;w.u64(bytecode.size());image+=w.bytes;image+=trailerMagic;return image;
}
fs::path findRuntime(const fs::path& self,const string& target) {
    if(target==hostTarget)return self;
    string name=target=="windows-x64"?"uma.exe":"uma";
    vector<fs::path> roots={self.parent_path(),self.parent_path().parent_path(),self.parent_path().parent_path().parent_path()};
    for(const auto& root:roots)for(const string& dir:{string("runtimes"),string("bin")}){
        fs::path p=root/dir/target/name;if(fs::is_regular_file(p))return p;
    }
    throw invalid_argument("Runtime "+target+" nao encontrado. Reinstale o pacote completo para compilar para os dois sistemas.");
}
string shellQuote(const string& s) {string out="'";for(char c:s){if(c=='\'')out+="'\\''";else out+=c;}return out+"'";}
#ifdef _WIN32
wstring wide(const string& s) {
    if(s.empty())return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
    if(!n)throw invalid_argument("Texto UTF-8 invalido.");
    wstring out(static_cast<size_t>(n),L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),out.data(),n);return out;
}
string narrow(const wstring& s) {
    if(s.empty())return {};
    int n=WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);
    string out(static_cast<size_t>(n),'\0');WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),out.data(),n,nullptr,nullptr);return out;
}
fs::path homePath() {
    PWSTR out=nullptr;HRESULT h=SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&out);if(FAILED(h))throw runtime_error("Nao foi possivel localizar LocalAppData.");
    fs::path p(out);CoTaskMemFree(out);return p;
}
void windowsPath(const fs::path& directory,bool add) {
    HKEY key;DWORD created;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Environment",0,nullptr,0,KEY_READ|KEY_WRITE,nullptr,&key,&created)!=ERROR_SUCCESS)throw runtime_error("Nao foi possivel alterar o PATH do usuario.");
    struct Close{HKEY k;~Close(){RegCloseKey(k);}}close{key};DWORD type=REG_EXPAND_SZ,size=0;
    LONG status=RegQueryValueExW(key,L"Path",nullptr,&type,nullptr,&size);wstring current;
    if(status!=ERROR_SUCCESS && status!=ERROR_FILE_NOT_FOUND)throw runtime_error("Falha ao consultar PATH.");
    if(status==ERROR_SUCCESS){if(type!=REG_SZ && type!=REG_EXPAND_SZ)throw runtime_error("Tipo de PATH inesperado.");vector<wchar_t> data(size/sizeof(wchar_t)+1,L'\0');if(RegQueryValueExW(key,L"Path",nullptr,&type,reinterpret_cast<BYTE*>(data.data()),&size)!=ERROR_SUCCESS)throw runtime_error("Falha ao ler PATH.");current=data.data();}
    auto normalize=[](wstring s){while(!s.empty() && (s.back()==L'\\' || s.back()==L'/'))s.pop_back();transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});return s;};
    wstring wanted=directory.wstring(), result;wstringstream in(current);wstring item;bool found=false;
    while(getline(in,item,L';')){wstring trim=item;if(trim.size()>1 && trim.front()==L'"' && trim.back()==L'"')trim=trim.substr(1,trim.size()-2);bool same=normalize(trim)==normalize(wanted);if(same){found=true;if(!add)continue;}if(!item.empty()){if(!result.empty())result+=L';';result+=item;}}
    if(add && !found){if(!result.empty())result+=L';';result+=wanted;}
    if(RegSetValueExW(key,L"Path",0,type,reinterpret_cast<const BYTE*>(result.c_str()),static_cast<DWORD>((result.size()+1)*sizeof(wchar_t)))!=ERROR_SUCCESS)throw runtime_error("Falha ao gravar PATH.");
    DWORD_PTR ignored;SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,reinterpret_cast<LPARAM>(L"Environment"),SMTO_ABORTIFHUNG,1000,&ignored);
}
#else
fs::path homePath(){const char* value=getenv("HOME");if(!value || !*value)throw runtime_error("HOME nao definido. Informe --prefix.");return pathOf(value);}
void linuxPath(const fs::path& prefix,bool add) {
    fs::path home=homePath();const string begin="# >>> UmaScript PATH >>>",end="# <<< UmaScript PATH <<<";
    const string block=begin+"\nexport PATH="+shellQuote(utf8(prefix))+":\"$PATH\"\n"+end+"\n";
    vector<fs::path> profiles={home/".profile",home/".bashrc"};
    if(fs::exists(home/".zshrc"))profiles.push_back(home/".zshrc");
    for(const auto& file:profiles){string s=fs::exists(file)?readFile(file,4u*1024u*1024u):"";size_t a=s.find(begin);if(a!=string::npos){size_t b=s.find(end,a);if(b==string::npos)throw runtime_error("Bloco PATH incompleto em "+utf8(file));b+=end.size();if(b<s.size() && s[b]=='\n')++b;s.erase(a,b-a);}
        if(!add && a==string::npos)continue;
        if(add){if(!s.empty() && s.back()!='\n')s+='\n';s+=block;}
        writeFile(fs::exists(file)?fs::canonical(file):file,s);
    }
}
#endif
fs::path defaultPrefix(){
#ifdef _WIN32
    return homePath()/"Programs"/"UmaScript";
#else
    return homePath()/".local"/"opt"/"umascript";
#endif
}
fs::path packageRoot(const fs::path& self) {
    for(auto p=self.parent_path();!p.empty();){if(fs::exists(p/"examples") && fs::exists(p/"bin"))return p;auto parent=p.parent_path();if(parent==p)break;p=parent;}
    return self.parent_path();
}
void install(const fs::path& self,const fs::path& prefix,const fs::path& root,bool path=true) {
    fs::path target=absPath(prefix), destination=target/exeName;
    if(target==target.root_path())throw invalid_argument("Escolha uma subpasta para instalar a UmaScript.");
    if(fs::exists(target) && !fs::is_empty(target) && !fs::exists(target/"umascript-install.txt"))throw invalid_argument("O destino contem arquivos de outra instalacao. Escolha uma pasta vazia.");
    fs::create_directories(target);
    vector<string> installed;
    auto record=[&](const fs::path& dest){installed.push_back(utf8(fs::relative(dest,target)));};
    if(!samePath(self,destination))writeFile(destination,readFile(self),true);
    record(destination);
    for(const string& name:{string("linux-x64"),string("windows-x64")}){
        string file=name=="windows-x64"?"uma.exe":"uma";fs::path src=root/"bin"/name/file;
        if(!fs::exists(src))src=root/"runtimes"/name/file;
        if(fs::is_regular_file(src)){fs::path dest=target/"runtimes"/name/file;fs::create_directories(dest.parent_path());if(!samePath(src,dest))writeFile(dest,readFile(src),name=="linux-x64");record(dest);}
        auto stamp=src.parent_path()/"version.txt";if(fs::is_regular_file(stamp)){auto dest=target/"runtimes"/name/"version.txt";fs::create_directories(dest.parent_path());if(!samePath(stamp,dest))writeFile(dest,readFile(stamp));record(dest);}
    }
    for(const string& name:{string("examples"),string("docs"),string("stdlib"),string("src"),string("vscode-extension"),string("toolchain")})if(fs::is_directory(root/name) && !samePath(root/name,target/name)){
        for(const auto& item:fs::recursive_directory_iterator(root/name))if(item.is_regular_file()){
            auto dest=target/name/fs::relative(item.path(),root/name);fs::create_directories(dest.parent_path());writeFile(dest,readFile(item.path()));record(dest);
        }
    }
    for(const string& file:{string("README.md"),string("LICENSE"),string("palavras.toml")})if(fs::is_regular_file(root/file)){
        if(!samePath(root/file,target/file))writeFile(target/file,readFile(root/file));
        record(target/file);
    }
    string manifest="UmaScript 2\n"+utf8(target)+"\n";
    for(const auto& name:installed)manifest+=name+"\n";
    writeFile(target/"umascript-install.txt",manifest);
    if(path){
#ifdef _WIN32
        windowsPath(target,true);
#else
        linuxPath(target,true);
#endif
    }
    cout<<"UmaScript "<<version<<" instalada em "<<utf8(target)<<"\n";
    if(path)cout<<"Feche e reabra o terminal. Depois execute: uma --version\n";
    cout<<"Exemplo: uma new meu_projeto\n";
}
void uninstall(const fs::path& self,const fs::path& prefix,bool onlyPath) {
    fs::path target=absPath(prefix);if(!fs::exists(target/"umascript-install.txt"))throw invalid_argument("A pasta nao e uma instalacao UmaScript reconhecida.");
    istringstream manifest(readFile(target/"umascript-install.txt"));string line;
    if(!getline(manifest,line) || line!="UmaScript 2" || !getline(manifest,line) || !samePath(pathOf(line),target))throw invalid_argument("Manifesto de instalacao invalido.");
    vector<fs::path> files;
    while(getline(manifest,line)){
        fs::path rel=pathOf(line);if(rel.empty() || rel.is_absolute())throw invalid_argument("Manifesto de instalacao invalido.");
        for(const auto& part:rel)if(part==".." || part==".")throw invalid_argument("Manifesto de instalacao invalido.");
        auto file=target/rel;
        for(auto dir=file.parent_path();dir!=target && !dir.empty();dir=dir.parent_path())if(fs::is_symlink(dir))throw invalid_argument("Pasta instalada foi substituida por um link. Remova-a manualmente.");
        files.push_back(file);
    }
#ifdef _WIN32
    windowsPath(target,false);
#else
    (void)self;
    linuxPath(target,false);
#endif
    if(!onlyPath){
        set<fs::path> directories;
        for(const auto& file:files){
#ifdef _WIN32
            if(samePath(file,self))continue;
#endif
            if(fs::is_directory(file))throw invalid_argument("Arquivo instalado foi substituido por uma pasta: "+utf8(file));
            fs::remove(file);
            for(auto dir=file.parent_path();dir!=target && !dir.empty();dir=dir.parent_path())directories.insert(dir);
        }
        for(auto it=directories.rbegin();it!=directories.rend();++it){error_code e;fs::remove(*it,e);}
        fs::remove(target/"umascript-install.txt");error_code e;fs::remove(target,e);
    }
    cout<<"UmaScript removida do PATH"<<(onlyPath?".\n":" e desinstalada.\n");
}
void usage(){cout<<
"UmaScript 3.0.0 | compilador estatico, bytecode e AOT nativo\n"
"Uso: uma <comando> [opcoes]\n\n"
"  new NOME                         Cria NOME/main.uma\n"
"  run ARQUIVO.uma|ARQUIVO.umac      Compila/executa sem ferramentas externas\n"
"  check ARQUIVO                    Valida sem executar\n"
"  compile ARQUIVO.uma [-o SAIDA]    Gera bytecode portavel .umac\n"
"  build ARQUIVO [-o SAIDA]          Gera um executavel autossuficiente\n"
"        --native [--cc CAMINHO]    Gera codigo de maquina via GCC/Clang -O3\n"
"        --debug                    AOT com simbolos de depuracao e -O0\n"
"        [--target windows-x64|linux-x64]\n"
"  emit-cpp ARQUIVO [-o SAIDA]       Exporta C++17 tipado autossuficiente\n"
"  inspect ARQUIVO [--stdin]         Indice JSON para ferramentas\n"
"  check ARQUIVO --json [--stdin]   Diagnosticos estruturados para editores\n"
"  lsp                              Servidor LSP por entrada/saida padrao\n"
"  fmt ARQUIVO [--dialeto=classico|uma] Formata e converte vocabulario\n"
"  repl                             Console interativo sem dependencias\n"
"  test PASTA                       Executa arquivos *.test.uma\n"
"  disasm ARQUIVO [-o SAIDA]         Mostra as instrucoes compiladas\n"
"  install [--prefix PASTA]          Instala e adiciona ao PATH do usuario\n"
"  uninstall [--prefix PASTA]        Remove a instalacao (Linux)\n"
"  --version                        Mostra a versao\n"
"  --help                           Mostra esta ajuda\n\n"
"Execucao: --max-steps N (0 = ilimitado), --max-depth N (1..4096).\n"
"Atalhos: uma programa.uma | uma --check programa.uma\n"
"O modo --native gera operacoes tipadas sem VM; requer GCC/Clang.\n"
"O modo bytecode inclui uma VM e dispensa compiladores externos.\n"
"Binarios incluidos: Windows x64 e Linux x86-64.\n";}
struct Options {string command="run",input,output,target=hostTarget,prefix,root,cc,dialect;vector<string> programArgs;bool dev=false,noCache=false,check=false,noPath=false,onlyPath=false,native=false,debug=false,json=false,fromStdin=false;uint64_t maxSteps=0;size_t maxDepth=1024;};
uint64_t number(const string& n,const string& arg){if(n.empty() || n.find_first_not_of("0123456789")!=string::npos)throw invalid_argument(arg+" exige inteiro nao negativo.");return stoull(n);}
Options options(const vector<string>& args){
    Options o;size_t i=1;const set<string> commands={"run","check","compile","build","disasm","new","install","uninstall","emit-cpp","inspect","test","fmt","repl"};
    if(i<args.size() && (commands.count(args[i]) || args[i]=="lsp"))o.command=args[i++];
    for(;i<args.size();++i){string a=args[i];auto value=[&](){if(++i>=args.size())throw invalid_argument(a+" exige um valor.");return args[i];};
        if(a=="--"){for(++i;i<args.size();++i)o.programArgs.push_back(args[i]);break;}
        if(a.rfind("--dialeto=",0)==0)o.dialect=a.substr(10);
        else if(a=="--dialeto")o.dialect=value();
        else if(a=="--check")o.check=true;
        else if(a=="-o" || a=="--output")o.output=value();
        else if(a=="--target")o.target=value();
        else if(a=="--prefix")o.prefix=value();
        else if(a=="--package-root")o.root=value();
        else if(a=="--native")o.native=true;
        else if(a=="--dev")o.dev=true;
        else if(a=="--no-cache")o.noCache=true;
        else if(a=="--debug")o.debug=true;
        else if(a=="--json")o.json=true;
        else if(a=="--stdin")o.fromStdin=true;
        else if(a=="--cc")o.cc=value();
        else if(a=="--no-path")o.noPath=true;
        else if(a=="--path-only")o.onlyPath=true;
        else if(a=="--max-steps")o.maxSteps=number(value(),a);
        else if(a=="--max-depth"){auto n=number(value(),a);if(n<1 || n>4096)throw invalid_argument("--max-depth deve estar entre 1 e 4096.");o.maxDepth=static_cast<size_t>(n);}
        else if(!a.empty() && a[0]=='-')throw invalid_argument("Opcao desconhecida: "+a);
        else {if(!o.input.empty())throw invalid_argument("Informe um unico arquivo ou nome de projeto.");o.input=a;}
    }
    if(o.target!="windows-x64" && o.target!="linux-x64")throw invalid_argument("Alvo desconhecido: "+o.target);
    if(o.check){if(o.command!="run" && o.command!="check")throw invalid_argument("--check exige run ou check.");o.command="check";}
    if(!o.output.empty() && o.command!="build" && o.command!="compile" && o.command!="disasm" && o.command!="emit-cpp")throw invalid_argument("-o exige build, compile, emit-cpp ou disasm.");
    if(o.native && o.command!="build")throw invalid_argument("--native exige build.");
    if((o.dev || o.noCache) && !o.native)throw invalid_argument("--dev e --no-cache exigem build --native.");
    if(o.debug && !o.native)throw invalid_argument("--debug exige --native.");
    if(!o.cc.empty() && !o.native)throw invalid_argument("--cc exige --native.");
    if(o.fromStdin && o.command!="check" && o.command!="inspect")throw invalid_argument("--stdin exige check ou inspect.");
    if(o.json && o.command!="check" && o.command!="inspect")throw invalid_argument("--json exige check ou inspect.");
    return o;
}
int mainApp(const vector<string>& args){
    uma_native::configureStack();signal(SIGINT,interrupt);string path,source;map<string,string> sourceFiles;bool json=find(args.begin(),args.end(),"--json")!=args.end() || (args.size()>1 && args[1]=="inspect");
    try {
        fs::path self=ownExecutable(args.empty()?"uma":args[0]);
        // Executaveis de programas executam somente o bytecode embutido.
        string payload;size_t imageSize=0;
        if(extractPayload(readFile(self),payload,imageSize)){
            vector<string> programArgs={args[0]};bool literalArguments=false;for(size_t i=1;i<args.size();++i){if(!literalArguments && args[i]=="--"){literalArguments=true;continue;}if(!literalArguments && (args[i]=="--max-steps" || args[i]=="--max-depth")){programArgs.push_back(args[i]);if(++i>=args.size())throw invalid_argument("Limite exige um valor.");programArgs.push_back(args[i]);}else uma_native::arguments.push_back(args[i]);}Options limits=options(programArgs);
            if(limits.command!="run" || !limits.input.empty() || !limits.output.empty())throw invalid_argument("O programa aceita apenas --max-steps N e --max-depth N.");
            Module m=deserialize(payload);path=m.sourceName;VM vm(m);vm.maxSteps=limits.maxSteps;vm.maxDepth=limits.maxDepth;vm.run();return 0;
        }
        if(args.size()==2 && (args[1]=="--help" || args[1]=="-h" || args[1]=="help")){usage();return 0;}
        if(args.size()==2 && (args[1]=="--version" || args[1]=="version")){cout<<"UmaScript "<<version<<" ("<<hostTarget<<", bytecode + AOT tipado C++17)\n";return 0;}
        if(args.size()==1){cerr<<"Uso: uma <comando> [arquivo]\nUse uma --help para mais opcoes.\n";return 2;}
        Options o=options(args);
        if(o.command=="lsp"){
#ifdef _WIN32
            _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
            return LanguageServer{packageRoot(self)}.run();
        }
        if(o.command=="install" || o.command=="uninstall"){
            if(!o.input.empty())throw invalid_argument("Use --prefix PASTA para escolher a instalacao.");
            fs::path prefix=o.prefix.empty()?defaultPrefix():pathOf(o.prefix);
            if(o.command=="install")install(self,prefix,o.root.empty()?packageRoot(self):pathOf(o.root),!o.noPath);else uninstall(self,prefix,o.onlyPath);return 0;
        }
        uma_native::arguments=o.programArgs;
        if(o.command=="repl")return repl(packageRoot(self));
        if(o.command=="fmt"){if(o.input.empty())throw invalid_argument("fmt exige arquivo .uma.");auto file=pathOf(o.input);auto original=readFile(file);writeFile(file,formatSource(original,o.dialect));cout<<"Formatado: "<<o.input<<'\n';return 0;}
        if(o.input.empty())throw invalid_argument("Informe um arquivo .uma/.umac ou o nome do projeto.");
        if(o.command=="test"){
            auto directory=pathOf(o.input);if(!fs::is_directory(directory))throw invalid_argument("test exige uma pasta.");vector<fs::path> files;
            for(const auto& item:fs::recursive_directory_iterator(directory))if(item.is_regular_file()){string n=item.path().filename().u8string();if(n.size()>=9 && n.substr(n.size()-9)==".test.uma")files.push_back(absPath(item.path()));}
            sort(files.begin(),files.end());if(files.empty())throw invalid_argument("Nenhum arquivo *.test.uma encontrado.");size_t failed=0;
            for(const auto& file:files){cout<<"Teste: "<<utf8(file)<<'\n'<<flush;vector<string> command={utf8(self),"run",utf8(file),"--max-steps",to_string(o.maxSteps),"--max-depth",to_string(o.maxDepth)};if(!o.root.empty()){command.push_back("--package-root");command.push_back(o.root);}if(runProcess(command))++failed;}
            cout<<files.size()-failed<<" passaram; "<<failed<<" falharam.\n";return failed?1:0;
        }
        if(o.command=="new"){
            fs::path dir=pathOf(o.input);if(fs::exists(dir))throw invalid_argument("O destino ja existe: "+o.input);fs::create_directories(dir);
            writeFile(dir/"main.uma","// Meu primeiro programa UmaScript\nwit nome = \"Sakura Bakushin O\";\nspeed vitorias = 3;\nbakushin(nome, \"venceu\", vitorias, \"corridas!\");\n");
            writeFile(dir/"README.md","# Projeto UmaScript\n\nExecute: `uma run main.uma`\n\nCompile: `uma build main.uma -o corrida"+(hostTarget=="windows-x64"?string(".exe"):string(""))+"`\n");
            cout<<"Projeto criado: "<<o.input<<"\nExecute: uma run "<<utf8(dir/"main.uma")<<"\n";return 0;
        }
        path=o.input;fs::path input=pathOf(path);string ext=utf8(input.extension());if(ext!=".uma" && ext!=".umac")throw invalid_argument("O arquivo deve ter extensao .uma ou .umac.");
        Module m;
        if(ext==".uma"){
            if(!o.fromStdin && !fs::is_regular_file(input))throw invalid_argument("Nao foi possivel abrir: "+o.input);
            auto root=o.root.empty()?packageRoot(self):pathOf(o.root);SourceLoader loader(root);loader.fromStdin=o.fromStdin;
            if(o.fromStdin){ostringstream text;text<<cin.rdbuf();loader.inputText=text.str();}
            vector<Token> tokens;try{tokens=loader.load(input,true);}catch(...){sourceFiles=move(loader.sources);throw;}
            sourceFiles=move(loader.sources);path=fs::weakly_canonical(absPath(input)).u8string();source=sourceFiles.at(path);
            Program p=Parser(move(tokens)).run();Checker(p).run();InitializationChecker(p).run();OwnershipChecker(p).run();
            if(o.command=="inspect"){cout<<metadata(p)<<'\n';return 0;}
            if(o.command=="emit-cpp" || o.native){
                if(o.native && o.target!=hostTarget)throw invalid_argument("AOT usa a plataforma local. Para compilacao cruzada, exporte emit-cpp e use uma ferramenta C++ para o alvo.");
                string runtime=readFile(root/"src"/"native_ops.hpp");string jsonInclude="#include \"json_runtime.hpp\"";auto jsonAt=runtime.find(jsonInclude);if(jsonAt!=string::npos)runtime.erase(jsonAt,jsonInclude.size());runtime+=readFile(root/"src"/"json_runtime.hpp");string part=readFile(root/"src"/"native_runtime.hpp");string include="#include \"native_ops.hpp\"";auto at=part.find(include);if(at!=string::npos)part.erase(at,include.size());runtime+='\n'+part;string pragma="#pragma once";while((at=runtime.find(pragma))!=string::npos)runtime.erase(at,pragma.size());
                string cpp=NativeEmitter(p,path).generate(runtime);fs::path dest=o.output.empty()?input:pathOf(o.output);if(o.output.empty())dest.replace_extension(o.command=="emit-cpp"?".cpp":hostTarget=="windows-x64"?".exe":"");
                if(samePath(dest,input) || samePath(dest,self))throw invalid_argument("A saida nao pode substituir a entrada ou o compilador.");
                for(const auto& file:sourceFiles)if(samePath(dest,pathOf(file.first)))throw invalid_argument("A saida nao pode substituir um modulo de codigo-fonte.");
                if(o.command=="emit-cpp"){writeFile(dest,cpp);cout<<"C++17 tipado: "<<utf8(dest)<<'\n';return 0;}
                string bundled=o.cc;if(hostTarget=="windows-x64" && bundled.empty() && !getenv("UMA_CXX") && fs::is_regular_file(root/"toolchain"/"bin"/"g++.exe"))bundled=utf8(root/"toolchain"/"bin"/"g++.exe");string cc=findCxx(bundled);if(pathOf(cc).has_parent_path())cc=utf8(absPath(pathOf(cc)));auto stamp=to_string(chrono::high_resolution_clock::now().time_since_epoch().count());auto temp=absPath(dest).parent_path()/("_uma_native_"+stamp);auto cfile=temp;cfile+=".cpp";auto binary=temp;binary+=hostTarget=="windows-x64"?".exe":".out";
                struct Cleanup{fs::path a,b;~Cleanup(){error_code e;fs::remove(a,e);fs::remove(b,e);}}cleanup{cfile,binary};
                vector<string> command={cc,"-std=c++17",o.debug?"-O0":o.dev?"-O1":"-O3","-fno-fast-math","-finput-charset=UTF-8","-fexec-charset=UTF-8"};if(o.debug)command.push_back("-g");command.insert(command.end(),{"-static","-static-libgcc","-static-libstdc++"});
#ifdef _WIN32
                command.push_back("-Wl,--stack,268435456");command.push_back("-Wl,--no-insert-timestamp");
#endif
                if(!o.noCache && pathOf(cc).filename().u8string().find("g++")!=string::npos){
                    string identity=runtime+cc;for(auto& flag:command)identity+=flag;error_code ec;auto time=fs::last_write_time(pathOf(cc),ec);if(!ec)identity+=to_string(time.time_since_epoch().count());
                    auto key=to_string(uma_native::hashText(identity));auto cache=absPath(input).parent_path()/".uma"/"cache";fs::create_directories(cache);auto header=cache/("runtime-"+key+".hpp"),pch=header;pch+=".gch";
                    if(!fs::is_regular_file(header) || !fs::is_regular_file(pch)){
                        writeFile(header,runtime);auto temporary=cache/("pch-"+stamp+".gch");auto flags=command;flags.insert(flags.end(),{"-x","c++-header","-c",utf8(header),"-o",utf8(temporary)});int status=runProcess(flags);if(status){fs::remove(temporary,ec);throw runtime_error("Falha ao preparar cache do runtime.");}writeFile(pch,readFile(temporary,1024u*1024u*1024u));fs::remove(temporary,ec);
                    }
                    cpp=NativeEmitter(p,path).generate("");command.insert(command.end(),{"-include",utf8(header)});
                }
                command.push_back("-ffile-prefix-map="+absPath(input).parent_path().generic_u8string()+"=.");
                command.push_back("-ffile-prefix-map="+absPath(root).generic_u8string()+"=umascript");
                writeFile(cfile,cpp);
                command.insert(command.end(),{utf8(cfile.filename()),"-o",utf8(binary.filename())});int result=runProcess(command,cfile.parent_path());if(result)throw runtime_error("Compilador C++ encerrou com codigo "+to_string(result)+". Exporte emit-cpp para inspecionar o codigo gerado.");
                writeFile(dest,readFile(binary),true);cout<<"Compilado: "<<utf8(dest)<<" [AOT nativo, "<<(o.debug?"debug":o.dev?"O1":"O3")<<", sem VM]\n";return 0;
            }
            m=compile(move(p),path);verify(m);
        }
        else {m=deserialize(readFile(input));path=m.sourceName;}
        if((o.native || o.command=="emit-cpp" || o.command=="inspect") && ext!=".uma")throw invalid_argument("Este comando exige codigo-fonte .uma.");
        if(o.command=="check"){cout<<(o.json?"{\"diagnostics\":[]}\n":"Treino validado: sintaxe, tipos e bytecode corretos.\n");return 0;}
        if(o.command=="run"){VM vm(m);vm.maxSteps=o.maxSteps;vm.maxDepth=o.maxDepth;vm.run();return 0;}
        if(o.command=="disasm"){
            ostringstream out;disassemble(m,out);if(o.output.empty())cout<<out.str();else {fs::path dest=pathOf(o.output);if(samePath(dest,input) || samePath(dest,self))throw invalid_argument("A saida nao pode substituir a entrada ou o compilador.");for(const auto& file:sourceFiles)if(samePath(dest,pathOf(file.first)))throw invalid_argument("A saida nao pode substituir um modulo de codigo-fonte.");writeFile(dest,out.str());}return 0;
        }
        fs::path dest;
        if(!o.output.empty())dest=pathOf(o.output);
        else {dest=input;dest.replace_extension(o.command=="compile"?".umac":o.target=="windows-x64"?".exe":"");}
        if(samePath(dest,input) || samePath(dest,self))throw invalid_argument("A saida nao pode substituir a entrada ou o compilador.");
        for(const auto& file:sourceFiles)if(samePath(dest,pathOf(file.first)))throw invalid_argument("A saida nao pode substituir um modulo de codigo-fonte.");
        string bytecode=serialize(m);
        if(o.command=="compile")writeFile(dest,bytecode);
        else {auto runtime=findRuntime(self,o.target);if(samePath(dest,runtime))throw invalid_argument("A saida nao pode substituir o runtime.");
            if(o.target!=hostTarget){auto stamp=runtime.parent_path()/"version.txt";if(!fs::is_regular_file(stamp) || readFile(stamp).rfind(version,0)!=0)throw invalid_argument("Runtime do alvo desatualizado; use o pacote desta versao.");}
            writeFile(dest,buildImage(runtime,bytecode,o.target),o.target=="linux-x64");}
        cout<<"Compilado: "<<utf8(dest)<<(o.command=="build"?" ["+o.target+", runtime embutido]":" [bytecode portavel]")<<"\n";return 0;
    }catch(const uma_native::Exit& e){return e.code;}catch(const Error& e){
        if(!e.at.file.empty()){path=e.at.file;auto it=sourceFiles.find(path);if(it!=sourceFiles.end())source=it->second;}
        if(json)cout<<"{\"diagnostics\":[{\"file\":"<<jsonString(path)<<",\"line\":"<<e.at.line<<",\"column\":"<<e.at.col<<",\"phase\":"<<jsonString(e.phase)<<",\"code\":"<<jsonString(e.code())<<",\"message\":"<<jsonString(e.what())<<"}]}\n";else diagnostic(e,path,source);return e.phase=="interrupcao"?130:1;}
    catch(const bad_alloc&){cerr<<"Memoria insuficiente para concluir o treino.\n";return 1;}
    catch(const exception& e){if(json)cout<<"{\"diagnostics\":[{\"file\":"<<jsonString(path)<<",\"line\":1,\"column\":1,\"phase\":\"ferramenta\",\"message\":"<<jsonString(e.what())<<"}]}\n";else cerr<<"UmaScript: "<<e.what()<<'\n';return 2;}
}
} // namespace uma

#ifdef _WIN32
int wmain(int argc,wchar_t** argv){SetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);vector<string> args;for(int i=0;i<argc;++i)args.push_back(uma::narrow(argv[i]));return uma::mainApp(args);}
#else
int main(int argc,char** argv){vector<string> args;for(int i=0;i<argc;++i)args.emplace_back(argv[i]);return uma::mainApp(args);}
#endif


