# CMake script to generate BuildInfo.h on every compilation
set(BUILD_NUMBER "1")
if(EXISTS "${BUILD_NUMBER_FILE}")
    file(READ "${BUILD_NUMBER_FILE}" BUILD_NUMBER_CONTENT)
    string(STRIP "${BUILD_NUMBER_CONTENT}" BUILD_NUMBER_CONTENT)
    if(BUILD_NUMBER_CONTENT MATCHES "^[0-9]+$")
        math(EXPR BUILD_NUMBER "${BUILD_NUMBER_CONTENT} + 1")
    endif()
endif()
file(WRITE "${BUILD_NUMBER_FILE}" "${BUILD_NUMBER}")

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

# Generate the header file content
file(WRITE "${BUILD_INFO_OUT}" 
"#pragma once
#define BUILD_VERSION \"0.0.1\"
#define BUILD_NUMBER \"${BUILD_NUMBER}\"
#define BUILD_TIMESTAMP \"${BUILD_TIMESTAMP}\"
")
