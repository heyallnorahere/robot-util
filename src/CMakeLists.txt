cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ROBOT_UTIL_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(robot-util ${ROBOT_UTIL_SRC})

pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(CJSON REQUIRED libcjson)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

target_link_libraries(robot-util
    ${GPIOD_LIBRARIES}
    ${CURL_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    pthread)

target_include_directories(robot-util PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GPIOD_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS})
