cmake_minimum_required(VERSION 3.12)

project(log-viewer CXX)

find_program(CCACHE ccache)
if(CCACHE)
    message(STATUS "Using ccache: ${CCACHE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
endif()

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/Mrokkk/FTXUI.git
    GIT_TAG main
)

FetchContent_Declare(rapidfuzz
    GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
    GIT_TAG main
)

FetchContent_Declare(re2
    GIT_REPOSITORY https://github.com/google/re2.git
    GIT_TAG main
)

FetchContent_MakeAvailable(ftxui)
FetchContent_MakeAvailable(rapidfuzz)
FetchContent_MakeAvailable(re2)

include(FindPkgConfig)
pkg_check_modules(BACKTRACE REQUIRED libbacktrace)

option(BUILD_TESTS "Build tests" OFF)
option(OPTIMIZE "Enable compiler optimizations" ON)
option(SANITIZERS "Enable ASan and UBsan" OFF)
option(COVERAGE "Enable collecting code coverage in tests" OFF)

message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Enable compiler optimizations: ${OPTIMIZE}")
message(STATUS "Enable ASan and UBsan: ${SANITIZERS}")
message(STATUS "Enable collecting coverage in tests: ${COVERAGE}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-pipe -std=c++23 -ggdb3 -Wall -Wextra -Werror -Wno-missing-field-initializers -fdiagnostics-color=always")

if(OPTIMIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

if(SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -Wl,-lasan -fno-omit-frame-pointer")
endif()

add_executable(${PROJECT_NAME}

    src/core/alias.cpp
    src/core/app.cpp
    src/core/argparse.cpp
    src/core/command.cpp
    src/core/commands/echo.cpp
    src/core/commands/get.cpp
    src/core/commands/help.cpp
    src/core/commands/map.cpp
    src/core/commands/open.cpp
    src/core/commands/quit.cpp
    src/core/commands/set.cpp
    src/core/commands/source.cpp
    src/core/commands/toggle.cpp
    src/core/context.cpp
    src/core/dirs.cpp
    src/core/file.cpp
    src/core/file_load.cpp
    src/core/fuzzy.cpp
    src/core/grep.cpp
    src/core/input.cpp
    src/core/interpreter.cpp
    src/core/lexer.cpp
    src/core/logger.cpp
    src/core/mode.cpp
    src/core/type.cpp
    src/core/variable.cpp
    src/main.cpp
    src/sys/posix.cpp
    src/ui/command_line.cpp
    src/ui/event_handler.cpp
    src/ui/ftxui.cpp
    src/ui/grepper.cpp
    src/ui/main_view.cpp
    src/ui/picker.cpp
    src/ui/status_line.cpp
    src/ui/ui_component.cpp
    src/ui/view.cpp
    src/utils/math.cpp
    src/utils/string.cpp

)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${BACKTRACE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ftxui::component
    rapidfuzz::rapidfuzz
    re2::re2
)

add_custom_target(run
    COMMAND ./${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
