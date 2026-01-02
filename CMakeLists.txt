cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

set(CMAKE_CXX_STANDARD 17)

# vcpkg automatically handles the search for curl
find_package(CURL REQUIRED)

add_executable(thunderos_auth thunderos_verify.cpp)

# Link the libraries found by vcpkg
target_link_libraries(thunderos_auth
    PRIVATE
    CURL::libcurl
    log
    android
)
