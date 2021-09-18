cmake_minimum_required(VERSION 3.5)

set(BINARY_NAME backlight-toggle)
project(${BINARY_NAME} C)

set(CMAKE_C_STANDARD 99)

# For reference see https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
IF( CMAKE_BUILD_TYPE MATCHES Debug )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        -Wall \
        -Wextra \
        -Wconversion \
        -Wsign-conversion \
        -Wmain \
        -Wreturn-type \
        -pedantic-errors \
        -pedantic \
        -DDEBUG \
        -O2 \
        -g3 \
        -ggdb \
        ")
    message("Debug build enabled")
ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        -Wall \
        -Wextra \
        -Wconversion \
        -Wsign-conversion \
        -Wmain \
        -Wreturn-type \
        -pedantic-errors \
        -pedantic \
        -O3 \
        ")
    message("Release build enabled")
ENDIF()

include_directories(${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCES RELATIVE ${PROJECT_SOURCE_DIR} "src/*.c")
add_executable(${BINARY_NAME} ${SOURCES})
target_link_libraries(${BINARY_NAME} i2c)
