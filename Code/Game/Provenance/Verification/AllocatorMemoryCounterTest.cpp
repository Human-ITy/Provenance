#include "../../../Base/ThirdParty/rpmalloc/rpmalloc.h"
#include <cstdio>
#include <cstring>
int main()
{
    if(rpmalloc_initialize()!=0)return 1;rpmalloc_global_statistics_t before{},live{},after{};rpmalloc_global_statistics(&before);
    void* allocation=rpmalloc(32*1024*1024);if(!allocation)return 2;std::memset(allocation,1,32*1024*1024);rpmalloc_global_statistics(&live);rpfree(allocation);rpmalloc_global_statistics(&after);
    bool pass=live.mapped>before.mapped&&after.mapped<live.mapped&&after.mapped_total>=live.mapped_total&&after.mapped_total>after.mapped;
    std::printf("current mapping before/live/freed %.2f / %.2f / %.2f MiB; cumulative after free %.2f MiB: %s\n",double(before.mapped)/1048576,double(live.mapped)/1048576,double(after.mapped)/1048576,double(after.mapped_total)/1048576,pass?"PASS":"FAIL");
    rpmalloc_finalize();return pass?0:1;
}
