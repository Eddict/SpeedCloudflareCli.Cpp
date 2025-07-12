# nlohmann_json and json-schema-validator integration for SpeedCloudflareCli
#
# This snippet is to be included in your main CMakeLists.txt to add nlohmann_json and json-schema-validator
# for JSON Schema validation support.

include(FetchContent)

# nlohmann_json (header-only)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# json-schema-validator (header-only, depends on nlohmann_json)
FetchContent_Declare(
  json-schema-validator
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
  GIT_TAG 2.3.0
)
FetchContent_MakeAvailable(json-schema-validator)

# Link to your main target (replace SpeedCloudflareCli with your actual target name)
target_link_libraries(SpeedCloudflareCli PRIVATE nlohmann_json nlohmann_json_schema_validator)
