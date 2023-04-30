cmake_minimum_required(VERSION 3.1...3.25)
project(colorized-logs C)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckFunctionExists)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_FLAGS "-Wall -Werror=format-security ${CMAKE_C_FLAGS}")

CHECK_INCLUDE_FILES(pty.h HAVE_PTY_H)
CHECK_INCLUDE_FILES(libutil.h HAVE_LIBUTIL_H)
CHECK_INCLUDE_FILES(util.h HAVE_UTIL_H)
CHECK_LIBRARY_EXISTS(util openpty "" HAVE_LIB_UTIL)
if (HAVE_LIB_UTIL)
    set(CMAKE_REQUIRED_LIBRARIES util)
endif (HAVE_LIB_UTIL)
CHECK_FUNCTION_EXISTS(openpty HAVE_FUNC_OPENPTY)

add_executable(ansi2html ansi2html.c)
add_executable(ansi2txt ansi2txt.c)
add_executable(ttyrec2ansi ttyrec2ansi.c)
add_executable(pipetty pipetty.c signals.c)

add_custom_target(lesstty ALL
    DEPENDS pipetty
    COMMAND ln -sf pipetty lesstty
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "LINK lesstty -> pipetty")
set_target_properties(lesstty
    PROPERTIES ADDITIONAL_CLEAN_FILES lesstty)

IF (HAVE_LIB_UTIL)
    set(LIB_UTIL util)
ENDIF (HAVE_LIB_UTIL)

target_link_libraries(pipetty ${LIB_UTIL})
configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

install(TARGETS
    ansi2html
    ansi2txt
    ttyrec2ansi
    pipetty
    RUNTIME DESTINATION bin)
install(FILES
    ${CMAKE_BINARY_DIR}/lesstty
    DESTINATION bin)

install(FILES
    ansi2html.1
    ansi2txt.1
    ttyrec2ansi.1
    pipetty.1
    lesstty.1
    DESTINATION share/man/man1
)

enable_testing()
add_subdirectory(tests)
