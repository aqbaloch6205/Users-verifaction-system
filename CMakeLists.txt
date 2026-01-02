cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

set(CMAKE_CXX_STANDARD 17)

# CRITICAL FIX: Disable _FORTIFY_SOURCE to prevent __sendto_chk linker errors
# This ensures we use the standard sendto symbol available in all Android versions
add_compile_options(-U_FORTIFY_SOURCE)

# Force vcpkg to prefer static libraries
set(VCPKG_LIBRARY_LINKAGE static)

# Find curl via vcpkg
find_package(CURL REQUIRED)

add_executable(thunderos_auth thunderos_verify.cpp)

# Link libraries
# We invoke CURL::libcurl which handles OpenSSL and ZLib dependencies automatically
target_link_libraries(thunderos_auth
    PRIVATE
    CURL::libcurl
)

# Ensure the binary is self-contained (no dependency on device's libc++_shared.so)
set_target_properties(thunderos_auth PROPERTIES 
    LINK_FLAGS "-static-libstdc++ -Wl,--gc-sections"
)
