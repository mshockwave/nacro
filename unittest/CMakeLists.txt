enable_testing()
find_package(GTest REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(NacroUnittests
               TestNacroParser.cpp
               TestNacroExpanders.cpp)
target_link_libraries(NacroUnittests
                      GTest::GTest GTest::Main
                      Nacro)

add_custom_target(check-units
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/NacroUnittests
  DEPENDS NacroUnittests)
