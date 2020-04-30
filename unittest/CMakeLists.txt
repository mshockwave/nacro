enable_testing()
find_package(GTest REQUIRED)
include(GoogleTest)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(NacroUnittests
               TestNacroParser.cpp)
target_link_libraries(NacroUnittests
                      GTest::GTest GTest::Main
                      Nacro)

gtest_add_tests(NacroUnittests "" AUTO)
