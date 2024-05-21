cmake_minimum_required(VERSION 3.10)
project(YourProjectName VERSION 1.0)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the path to the directory containing nlohmann_jsonConfig.cmake
set(CMAKE_PREFIX_PATH /path/to/nlohmann_json/installation)

# Add dependencies
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${nlohmann_json_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    ${PROJECT_SOURCE_DIR}/main.cpp
    # Add other source files here
)

# Add executable target
add_executable(YourExecutableName ${SOURCES})

# Link libraries
target_link_libraries(YourExecutableName PRIVATE ${CURL_LIBRARIES})

# Install executable
install(TARGETS YourExecutableName DESTINATION bin)
