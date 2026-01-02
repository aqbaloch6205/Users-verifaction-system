cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

set(CMAKE_CXX_STANDARD 17)

# Force vcpkg to look for static libraries
set(VCPKG_LIBRARY_LINKAGE static)

find_package(CURL REQUIRED)

add_executable(thunderos_auth thunderos_verify.cpp)

# Link curl and force static runtime
target_link_libraries(thunderos_auth
    PRIVATE
    CURL::libcurl
)

# Ensure no dynamic dependencies on C++ runtime
set_target_properties(thunderos_auth PROPERTIES 
    LINK_FLAGS "-static-libstdc++"
)
