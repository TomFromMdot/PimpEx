



add_executable(cpp_test "../test/test_e.cpp")

enable_testing()
add_test(NAME TEST_XMLReader COMMAND $<TARGET_FILE:cpp_test>)
