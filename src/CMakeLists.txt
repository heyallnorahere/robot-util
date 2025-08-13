cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ROBOT_UTIL_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_executable(robot-util ${ROBOT_UTIL_SRC})
target_link_libraries(robot-util gpiod pthread cjson)
