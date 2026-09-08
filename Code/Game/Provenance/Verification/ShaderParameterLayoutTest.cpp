#include "Engine/Render/RenderShaderParameterLayout.h"
#include <array>
#include <cstdio>

struct Field { size_t m_parameterOffsetInBytes, m_parameterStrideInBytes; };
int main()
{
    unsigned checks=0,failures=0;
    auto check=[&](bool pass,char const* label){++checks;if(!pass){++failures;std::printf("FAIL %s\n",label);}};
    auto size=[](auto const& fields){return EE::Render::ShaderParameterAllocationSize(fields);};
    check(size(std::array<Field,1>{{{60,2}}})==64,"grass wind trailing half-word: 62 -> 64");
    check(size(std::array<Field,2>{{{60,2},{0,4}}})==64,"reflection order is irrelevant");
    check(size(std::array<Field,1>{{{60,4}}})==64,"already padded size remains 64");
    check(size(std::array<Field,1>{{{64,2}}})==96,"handle beyond block requires another block");
    check(size(std::array<Field,0>{})==0,"empty layout remains empty");
    for(size_t end=1;end<=4096;++end)
    {
        size_t allocation=size(std::array<Field,1>{{{end-1,1}}});
        check(allocation>=end&&allocation%32==0&&allocation-end<32,"minimal complete-block coverage");
    }
    // Emitted from CURRENT generated shader reflection, not a hand-copied layout.
    #include "ShaderParameterLayoutCases.inl"
    std::printf("Shader allocation: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
