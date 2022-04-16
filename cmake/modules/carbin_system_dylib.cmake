

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CARBIN_DYNAMIC_LIB ${CARBIN_DYNAMIC_LIB} rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CARBIN_DYNAMIC_LIB ${CARBIN_DYNAMIC_LIB}
            pthread
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework CoreData"
            "-framework CoreText"
            "-framework Security"
            "-framework Foundation"
            "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
            "-Wl,-U,_ProfilerStart"
            "-Wl,-U,_ProfilerStop")
endif ()