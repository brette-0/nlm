cmake_minimum_required(VERSION 3.10)
project(nlm VERSION 0.0)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(
    # Add paths to your include directories here
)

# Source files
set(SOURCES
    main.cpp
    # Add other source files here
)

# Add executable target
add_executable(nlm ${SOURCES})

# Link libraries if needed (e.g., for curl)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
target_link_libraries(nlm PRIVATE CURL::libcurl)

# Install executable
install(TARGETS nlm DESTINATION bin)
