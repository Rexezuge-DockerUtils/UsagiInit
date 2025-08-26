cmake_minimum_required(VERSION 3.15)
project(UsagiInit LANGUAGES C)

# Compiler Options                                                                                                │
add_compile_options(-Wall)

# C-Standard Configuration
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure Release Only Definitions
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(APP_NAME="${PROJECT_NAME}")
    add_compile_definitions(RELEASE_MODE)
endif()

# Configure Individual Definitions Based on Input
if(TERMINATE_ALL_PROCESSES STREQUAL "ON")
    add_compile_definitions(TERMINATE_ALL_PROCESSES)
endif()
if(REINITIALIZE_ON_ALL_SERVICE_TERMINATION STREQUAL "ON")
    add_compile_definitions(REINITIALIZE_ON_ALL_SERVICE_TERMINATION)
endif()

# Configure Library Sources (exclude main.c)
file(GLOB LIB_SOURCES "src/*.c" "src/*/*.c")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

# Configure Headers
file(GLOB HEADERS "include/*.h" "include/*/*.h" "src/*.h" "src/*/*.h")

# Core Library Containing Business Logic
add_library(usagi_lib STATIC ${LIB_SOURCES} ${HEADERS})

# Public Include Directories (Propagate to Dependents)
target_include_directories(usagi_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                            ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Main Executable
add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE usagi_lib)
