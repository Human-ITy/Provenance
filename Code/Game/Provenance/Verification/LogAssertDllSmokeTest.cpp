#include <cstdio>

// Exercise the published Base DLL's early-startup assertion reporting path.
// No logger initialization or graphics device is required for this path.
namespace EE::SystemLog
{
    __declspec(dllimport) void LogAssert(char const*,int,char const*);
}
int main()
{
    EE::SystemLog::LogAssert(__FILE__,__LINE__,"( parametersSizeInBytes % 32 ) == 0");
    EE::SystemLog::LogAssert(__FILE__,__LINE__,"literal %s %n %p");
    EE::SystemLog::LogAssert(__FILE__,__LINE__,"100% complete");
    EE::SystemLog::LogAssert(__FILE__,__LINE__,"ordinary assertion");
    std::puts("Published Base DLL: 4 assertion-format smoke calls returned without CRT failure");
    return 0;
}
