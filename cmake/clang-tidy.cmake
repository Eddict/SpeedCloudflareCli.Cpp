# Note: All static analysis and formatting is controlled by .clang-tidy and .clang-format in the project root.
# Do not add per-target or per-directory clang-tidy/format config elsewhere.

file(GLOB SRC_FILES "${USER_SRC_DIR}/*.cpp")
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    if(CMAKE_CROSSCOMPILING)
        set(CLANG_TIDY_TARGET_NAME clang-tidy-arm)
        set(CLANG_TIDY_COMMENT "Running clang-tidy on user sources using ARM build's compile_commands.json and .clang-tidy")
    else()
        set(CLANG_TIDY_TARGET_NAME clang-tidy-native)
        set(CLANG_TIDY_COMMENT "Running clang-tidy on user sources")
        set_target_properties(SpeedCloudflareCli PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
    add_custom_target(
        ${CLANG_TIDY_TARGET_NAME}
        COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            --extra-arg-before=-std=c++17
            ${SRC_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "${CLANG_TIDY_COMMENT}"
        VERBATIM
    )
endif()
