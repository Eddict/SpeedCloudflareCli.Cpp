# cmake/embed_json_as_raw_string.cmake
# Usage: cmake -DINPUT=... -DOUTPUT=... -DVAR=... -P embed_json_as_raw_string.cmake

file(READ "${INPUT}" CONTENT)
file(WRITE "${OUTPUT}" "#pragma once\nstatic const char* ${VAR} = R\"json(\n${CONTENT}\n)json\";\n")
