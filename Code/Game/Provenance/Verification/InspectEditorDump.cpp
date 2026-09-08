#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <string>
#include <vector>
#pragma comment(lib,"dbghelp.lib")
// Read-only local dump triage. Stack-word symbols are candidates, not unwinding.
int wmain(int argc,wchar_t** argv)
{
    if(argc!=3)return 2;
    auto f=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    auto map=CreateFileMappingW(f,nullptr,PAGE_READONLY,0,0,nullptr);auto base=(char*)MapViewOfFile(map,FILE_MAP_READ,0,0,0);if(!base)return 3;
    auto stream=[&](ULONG type){PMINIDUMP_DIRECTORY dir=nullptr;PVOID p=nullptr;ULONG bytes=0;MiniDumpReadDumpStream(base,type,&dir,&p,&bytes);return p;};
    auto modules=(MINIDUMP_MODULE_LIST*)stream(ModuleListStream);auto ex=(MINIDUMP_EXCEPTION_STREAM*)stream(ExceptionStream);if(!modules||!ex)return 4;
    HANDLE proc=GetCurrentProcess();SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);SymInitializeW(proc,argv[2],FALSE);
    for(ULONG i=0;i<modules->NumberOfModules;++i){auto& m=modules->Modules[i];auto name=(MINIDUMP_STRING*)(base+m.ModuleNameRva);std::wstring path(name->Buffer,name->Length/2);auto slash=path.find_last_of(L"\\/");std::wstring local=std::wstring(argv[2])+L"/"+path.substr(slash+1);SymLoadModuleExW(proc,nullptr,local.c_str(),nullptr,m.BaseOfImage,m.SizeOfImage,nullptr,0);}
    auto symbol=[&](DWORD64 addr){char storage[sizeof(SYMBOL_INFO)+2048]={};auto info=(SYMBOL_INFO*)storage;info->SizeOfStruct=sizeof(SYMBOL_INFO);info->MaxNameLen=2047;DWORD64 disp=0;if(!SymFromAddr(proc,addr,&disp,info))return false;std::printf("%llx %s + %llx",addr,info->Name,disp);IMAGEHLP_LINE64 line{};line.SizeOfStruct=sizeof(line);DWORD d=0;if(SymGetLineFromAddr64(proc,addr,&d,&line))std::printf(" %s:%lu",line.FileName,line.LineNumber);std::puts("");return true;};
    auto ctx=(CONTEXT*)(base+ex->ThreadContext.Rva);std::printf("Exception %lx thread %lu RIP ",ex->ExceptionRecord.ExceptionCode,ex->ThreadId);symbol(ctx->Rip);
    auto threads=(MINIDUMP_THREAD_LIST*)stream(ThreadListStream);
    for(ULONG i=0;threads&&i<threads->NumberOfThreads;++i){auto& t=threads->Threads[i];if(t.ThreadId!=ex->ThreadId)continue;size_t start=ctx->Rsp>=t.Stack.StartOfMemoryRange?size_t(ctx->Rsp-t.Stack.StartOfMemoryRange):0;for(size_t k=start;k+8<=t.Stack.Memory.DataSize&&k<start+4096;k+=8){auto address=*(DWORD64*)(base+t.Stack.Memory.Rva+k);symbol(address);}}
    SymCleanup(proc);UnmapViewOfFile(base);CloseHandle(map);CloseHandle(f);return 0;
}
