cmake_minimum_required(VERSION 3.15)

function(add_unit_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE usagi_lib cmocka::cmocka)
  target_include_directories(${name} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_preprocess)
add_unit_test(test_parser)
add_unit_test(test_services)
add_unit_test(test_guardian)
add_unit_test(test_utils)
add_unit_test(test_registration)

target_link_options(test_guardian PRIVATE -Wl,--wrap=fork,--wrap=execvpe,--wrap=execvp)
target_link_options(test_utils    PRIVATE -Wl,--wrap=getenv)
