#if _GAMES_FOR_WINDOWS
    // Use secure / developer version of Xbox network libraries
    #define _XBOX_SECURE    1
    /// Use IPPROTO_VDP protocol on Xbox
    #define _XBOX_VDP       1
#else
    // Always 0 on PC
    #define _XBOX_SECURE    0
    #define _XBOX_VDP       0
#endif
