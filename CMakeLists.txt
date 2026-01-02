cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

set(CMAKE_CXX_STANDARD 17)

# Force vcpkg to prefer static libraries
set(VCPKG_LIBRARY_LINKAGE static)

find_package(CURL REQUIRED)

add_executable(thunderos_auth thunderos_verify.cpp)

# Link libraries
target_link_libraries(thunderos_auth
    PRIVATE
    CURL::libcurl
)

# Static linking properties
set_target_properties(thunderos_auth PROPERTIES 
    LINK_FLAGS "-static-libstdc++ -Wl,--gc-sections"
)