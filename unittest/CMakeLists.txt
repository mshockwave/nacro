enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "Using GTest: ${GTEST_LIBRARIES}")
message(STATUS "Using GTestMain: ${GTEST_MAIN_LIBRARIES}")

include_directories(${CMAKE_SOURCE_DIR})

add_executable(NacroUnittests
               TestNacroParser.cpp
               TestNacroExpanders.cpp)
target_link_libraries(NacroUnittests
                      Nacro
                      ${GTEST_BOTH_LIBRARIES})

add_custom_target(check-units
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/NacroUnittests
  DEPENDS NacroUnittests)
