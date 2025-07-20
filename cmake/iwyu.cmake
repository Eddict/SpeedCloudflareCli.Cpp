# IWYU integration: add custom target using mapping file (fixed: use SRC_FILES for sources)
if(NOT CMAKE_CROSSCOMPILING)
    file(GLOB SRC_FILES "${USER_SRC_DIR}/*.cpp")
    find_program(IWYU_EXE NAMES include-what-you-use)
    if(IWYU_EXE)
        add_custom_target(
            iwyu
            COMMAND python3 /usr/local/bin/iwyu_tool.py
                -p ${CMAKE_BINARY_DIR}
                ${SRC_FILES}
                -- -I/usr/lib/gcc/x86_64-linux-gnu/12/include
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running include-what-you-use with mapping file for all user sources"
            VERBATIM
        )
    endif()
endif()

