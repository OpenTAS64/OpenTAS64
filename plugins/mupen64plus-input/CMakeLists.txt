add_library(OpenTAS64MupenInput SHARED Plugin.cpp)
target_compile_features(OpenTAS64MupenInput PRIVATE cxx_std_17)
set_target_properties(OpenTAS64MupenInput PROPERTIES
    OUTPUT_NAME "mupen64plus-input-opentas64"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
if(WIN32)
    target_link_libraries(OpenTAS64MupenInput PRIVATE ws2_32)
endif()
