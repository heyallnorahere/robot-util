cmake_minimum_required(VERSION 3.21)

set(ROBOT_UTIL_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
file(GLOB_RECURSE ROBOT_UTIL_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

list(REMOVE_ITEM ROBOT_UTIL_SRC ${ROBOT_UTIL_MAIN})
add_library(utillib STATIC ${ROBOT_UTIL_SRC})
add_executable(robot-util ${ROBOT_UTIL_MAIN})

pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(CJSON REQUIRED libcjson)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(CURSES REQUIRED ncurses)

target_link_libraries(utillib PUBLIC
    ${GPIOD_LIBRARIES}
    ${CURL_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${CURSES_LIBRARIES}
    pthread)

target_include_directories(utillib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GPIOD_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${CURSES_INCLUDE_DIRS})

target_link_libraries(robot-util PRIVATE utillib)
