cmake_minimum_required(VERSION 3.10)
project(ThunderOSAuth)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Search for system libraries provided by NDK
find_library(log-lib log)
find_library(android-lib android)

# Define the executable
add_executable(thunderos_auth thunderos_verify.cpp)

# Link necessary libraries
# Note: curl is usually provided via a prebuilt or static build in the workflow
target_link_libraries(thunderos_auth
    ${log-lib}
    ${android-lib}
    curl
)
