# nlohmann_json and json-schema-validator integration for SpeedCloudflareCli
#
# This snippet is to be included in your main CMakeLists.txt to add nlohmann_json and json-schema-validator
# for JSON Schema validation support.

set(JSON_VALIDATOR_SHARED_LIBS ON)

include(FetchContent)

# nlohmann_json (header-only)
FetchContent_Declare(
  nlohmann_json
  SYSTEM
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# json-schema-validator (header-only, depends on nlohmann_json)
FetchContent_Declare(
  json-schema-validator
  SYSTEM
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
  GIT_TAG 2.3.0
)
FetchContent_MakeAvailable(json-schema-validator)

# Link to your main target (replace SpeedCloudflareCli with your actual target name)
target_link_libraries(SpeedCloudflareCli PRIVATE nlohmann_json nlohmann_json_schema_validator)

target_include_directories(SpeedCloudflareCli PRIVATE
    ${json-schema-validator_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/_deps/json-schema-validator-src/src
)

# === Embed JSON schemas as C++ raw string headers ===
macro(embed_json_raw INPUT OUTPUT VAR)
  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -DINPUT=${INPUT}
      -DOUTPUT=${OUTPUT}
      -DVAR=${VAR}
      -P ${CMAKE_SOURCE_DIR}/cmake/embed_json_as_raw_string.cmake
  )
endmacro()

embed_json_raw(
  "${CMAKE_SOURCE_DIR}/schemas/result.schema.json"
  "${CMAKE_BINARY_DIR}/embedded_result_schema.h"
  embedded_result_schema
)
embed_json_raw(
  "${CMAKE_SOURCE_DIR}/schemas/summary.schema.json"
  "${CMAKE_BINARY_DIR}/embedded_summary_schema.h"
  embedded_summary_schema
)
