cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ROBOT_UTIL_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(robot-util ${ROBOT_UTIL_SRC})

pkg_check_modules(DBUS REQUIRED dbus-1)

target_link_libraries(robot-util gpiod pthread cjson curl ${DBUS_LIBRARIES})
target_include_directories(robot-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${DBUS_INCLUDE_DIRS})
