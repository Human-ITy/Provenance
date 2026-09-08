#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>

// Compile the current production LogAssert + Win32 TraceMessage bodies against
// test sinks. This tests both formatting calls without deliberately crashing
// the editor or requiring graphics/device initialization.
namespace EE::SystemLog
{
    enum class Severity { Error };
    void* g_pLog=nullptr;
    std::string traced,logged;
    void OutputDebugStringA(char const* text){traced=text;}
    void AddEntry(Severity,char const*,char const*,char const*,int,char const* format,...)
    {
        char buffer[2048];va_list args;va_start(args,format);
        vsnprintf_s(buffer,sizeof(buffer),sizeof(buffer)-1,format,args);
        va_end(args);logged=buffer;
    }
    #define EE_TRACE_MSG(...) TraceMessage(__VA_ARGS__)
    #include "LogAssertRegression.inl"
    #undef EE_TRACE_MSG
}
int main()
{
    using namespace EE::SystemLog;
    unsigned checks=0,failures=0;
    for(char const* text:{"( parametersSizeInBytes % 32 ) == 0","literal %s %n %p","100% complete","ordinary assertion"})
    {
        for(bool initialized:{false,true})
        {
            int dummy=0;g_pLog=initialized?&dummy:nullptr;traced.clear();logged.clear();
            LogAssert(__FILE__,__LINE__,text);
            ++checks;if(traced!=std::string(text)+"\r\n")++failures;
            ++checks;if(logged!=(initialized?std::string(text):std::string()))++failures;
        }
    }
    std::printf("Assertion format: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
