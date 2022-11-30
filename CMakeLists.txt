cmake_minimum_required(VERSION 3.14)
project(TimelineServer CXX)

# GoogleTest 需要至少 C++11
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# GoogleTest基础设置
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 添加子目录和头文件
include_directories("code")
file(GLOB TIMELINE_SERVER_SRC "code/*/*.cc")

include(GoogleTest)

file(GLOB TEST_FILE "test/*/*.cc")
# 遍历所有测试文件夹
foreach(test_file ${TEST_FILE})
  # 获取无后缀文件名作为编译目标
  get_filename_component(target ${test_file} NAME_WE)

  # 编译测试文件
  add_executable(
    ${target}
    ${test_file}
    ${TIMELINE_SERVER_SRC}
  )

  # 链接测试文件
  # 链接mysqlclient
  target_link_libraries(
    ${target}
    gtest_main
    mysqlclient
  )

  gtest_discover_tests(${target})
endforeach(test_file)