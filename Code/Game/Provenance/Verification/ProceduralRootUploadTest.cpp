// Targeted CPU reproduction of the procedural-root callback's alignment
// contract. Calls the real engine copy routine; this is not a GPU smoke test.
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <new>
namespace EE::Memory
{
    __declspec(dllimport) void CopyToWriteCombined(void* __restrict,void const* __restrict,size_t);
    __declspec(dllimport) void WriteCombinedBarrier();
}
int main()
{
    std::array<uint32_t,32> rootUpload{};
    for(size_t i=0;i<rootUpload.size();++i)rootUpload[i]=uint32_t(i*7919+17);
    auto callback=[rootUpload](uint8_t* dst,size_t size)
    {
        alignas(32) uint32_t alignedRootUpload[32];
        std::memcpy(alignedRootUpload,rootUpload.data(),sizeof(alignedRootUpload));
        EE::Memory::CopyToWriteCombined(dst,alignedRootUpload,size);
    };
    using Callback=decltype(callback);
    static_assert(alignof(Callback)<=4,"exercise ordinarily aligned captured storage");
    int failures=0;
    // Every possible 4-byte-aligned capture address modulo the required 32.
    for(size_t offset=0;offset<32;offset+=4)
    {
        alignas(32) uint8_t storage[sizeof(Callback)+32];
        auto* copied=new(storage+offset) Callback(callback);
        alignas(32) std::array<uint8_t,192> destination;destination.fill(0xA5);
        (*copied)(destination.data()+32,128);
        EE::Memory::WriteCombinedBarrier();
        if(std::memcmp(destination.data()+32,rootUpload.data(),128)!=0)++failures;
        for(size_t i=0;i<32;++i)if(destination[i]!=0xA5||destination[160+i]!=0xA5)++failures;
        copied->~Callback();
    }
    std::printf("Root upload: 8 capture alignments, payload + guard checks, %d failures\n",failures);
    return failures?1:0;
}
