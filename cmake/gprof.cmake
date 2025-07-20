# Option to enable gprof profiling instrumentation
option(ENABLE_GPROF "Enable gprof profiling instrumentation" OFF)
if(ENABLE_GPROF)
    message(STATUS "gprof instrumentation enabled: adding -pg to compile and link flags")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()
