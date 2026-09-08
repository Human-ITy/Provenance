#pragma once
//-------------------------------------------------------------------------

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    // The authority verifier compiles the Granite geometry sources directly.
    // This prevents the gate from silently testing a stale Game Runtime DLL.
    #define EE_GAME_API
#elif EE_DLL
    #ifdef ESOTERICA_GAME_RUNTIME
        #define EE_GAME_API __declspec(dllexport)
    #else
        #define EE_GAME_API __declspec(dllimport)
    #endif
#else
    #define EE_GAME_API
#endif
