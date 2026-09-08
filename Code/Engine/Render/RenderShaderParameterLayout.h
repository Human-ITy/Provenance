#pragma once
#include <cstddef>

namespace EE::Render
{
    // Reflection describes fields, not necessarily the trailing padding of the
    // packed HLSL struct. A final 16-bit handle can end two bytes before a word
    // boundary. Shader storage and addressing use complete 32-byte blocks.
    template<typename ParameterRange>
    constexpr size_t ShaderParameterAllocationSize( ParameterRange const& parameters )
    {
        size_t coveredBytes = 0;
        for ( auto const& parameter : parameters )
        {
            size_t const end = size_t( parameter.m_parameterOffsetInBytes ) + parameter.m_parameterStrideInBytes;
            if ( end > coveredBytes ) coveredBytes = end;
        }
        return ( ( coveredBytes + 31 ) / 32 ) * 32;
    }
}
