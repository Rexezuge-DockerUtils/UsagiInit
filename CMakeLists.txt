cmake_minimum_required(VERSION 3.15)
project(UsagiInit LANGUAGES C)

# C-Standard Configuration
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
