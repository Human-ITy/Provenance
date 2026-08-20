#include "../../Code/Applications/ProvenanceClient/Ei0aHandshake.h"
#include "../../Code/Applications/ProvenanceClient/MacroManifestAuthority.h"

#include <cstdio>
#include <string>

int main()
{
    std::string const hello=Ei0a::BuildMacroWorldGenesisHelloParams("ei1-hello");
    if(hello.find("\"requested_projection_modes\":[\"macro_manifest\",\"macro_page_v2\"]")==std::string::npos)
    {std::fprintf(stderr,"macro ClientHello FAIL\n");return 1;}
    MacroPageAuthority::Context context;
    auto const manifest=MacroManifestAuthority::Load(
        "Data\\Worldgen\\MacroAuthority\\macro_manifest.mcm",context);
    if(!manifest.valid||manifest.pages.size()!=25)
    {std::fprintf(stderr,"manifest FAIL: %s\n",manifest.failure.c_str());return 2;}
    for(auto const& item:manifest.pages)
    {
        auto const& binding=item.second;
        std::string const path="Data\\Worldgen\\MacroAuthority\\page_"+
            std::to_string(binding.ri)+"_"+std::to_string(binding.rj)+".mcp";
        auto const page=MacroPageAuthority::Load(path,binding.ri,binding.rj,context);
        std::string failure;
        if(!MacroManifestAuthority::ValidatePageBinding(manifest,binding,path,page,failure))
        {std::fprintf(stderr,"page %d,%d FAIL: %s\n",binding.ri,binding.rj,failure.c_str());return 3;}
    }
    std::printf("EI1_CPP_CLIENT PASS pages=%zu macro_source=ENGINE_WORLDGEN fallback=DISABLED\n",
                manifest.pages.size());
    return 0;
}
