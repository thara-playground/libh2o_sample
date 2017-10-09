cmake_minimum_required(VERSION 3.8)
project(libh2o_sample)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(libh2o_sample ${SOURCE_FILES})

include_directories(/usr/local/include)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_HOME)
    string(REGEX REPLACE "\n$" "" HOMEBREW_HOME "${HOMEBREW_HOME}")
    message(STATUS "Homebrew home path: ${HOMEBREW_HOME}")
    include_directories(${HOMEBREW_HOME}/include)

    find_library(libuv NAMES libuv.a)

    include_directories(/usr/local/opt/openssl/include)

    find_library(libssl NAMES libssl.a
            PATHS ${HOMEBREW_HOME}/opt/openssl/lib)
    find_library(libcrypto NAMES libcrypto.a
            PATHS ${HOMEBREW_HOME}/opt/openssl/lib)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

find_library(libh2o NAMES libh2o.a)


target_link_libraries(libh2o_sample ${libh2o} ${libuv} ${libssl} ${libcrypto})