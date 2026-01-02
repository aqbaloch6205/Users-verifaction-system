cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

set(CMAKE_CXX_STANDARD 17)

# Find curl via vcpkg
find_package(CURL REQUIRED)

add_executable(thunderos_auth thunderos_verify.cpp)

# We remove 'android' and 'log' to prevent "library not found" errors.
# We use -static-libstdc++ to ensure the C++ runtime is inside the binary.
target_link_libraries(thunderos_auth
    PRIVATE
    CURL::libcurl
)

set_target_properties(thunderos_auth PROPERTIES LINK_FLAGS "-static-libstdc++")
