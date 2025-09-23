function(add_coverage_targets executable run-target prefix source-dir)
    foreach(arg IN LISTS ARGN)
        set(exclude_string ${exclude_string} -e ${arg})
    endforeach()
    add_custom_target(clean-coverage
        COMMAND find ${CMAKE_BINARY_DIR} -name '*.gcda' -exec rm {} "\;"
        COMMENT "Cleaning coverage data")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_dependencies(${executable} clean-coverage)
        add_custom_target(llvm_coverage
            DEPENDS ${executable}
            COMMAND LLVM_PROFILE_FILE=tests.profdata ./${executable}
            COMMAND llvm-profdata merge -instr tests.profdata -o merged.profdata)
        add_custom_target(tests-cov
            DEPENDS llvm_coverage
            COMMAND llvm-cov report ./${executable} -instr-profile=merged.profdata
            COMMENT "Running LLVM coverage generating")
        add_custom_target(tests-cov-full
            DEPENDS llvm_coverage
            COMMAND llvm-cov show ./${executable} -instr-profile=merged.profdata -Xdemangler=llvm-cxxfilt
            COMMENT "Running LLVM coverage generating (full)")
    else()
        add_dependencies(${executable} clean-coverage)
        add_custom_target(${prefix}-cov
            COMMAND gcovr -r ${source-dir} ${CMAKE_BINARY_DIR}
            DEPENDS ${run-target}
            COMMENT "Running GCOVR coverage generating")
        add_custom_target(${prefix}-cov-html
            COMMAND gcovr --html --html-details -o ${prefix}-cov.html ${exclude_string} -r ${source-dir} ${CMAKE_BINARY_DIR}
            DEPENDS ${run-target}
            COMMENT "Running GCOVR coverage generating (HTML)")
    endif()
endfunction(add_coverage_targets)

pkg_check_modules(GTEST gtest REQUIRED)

add_executable(test

    main.cpp

    ${PROJECT_SOURCE_DIR}/src/core/interpreter/lexer.cpp
    ${PROJECT_SOURCE_DIR}/src/core/interpreter/object.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/buffer.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/time.cpp

    bitflag_tests.cpp
    buffer_tests.cpp
    hash_map_tests.cpp
    lexer_tests.cpp
    maybe_tests.cpp
    ring_buffer_tests.cpp
    trie_tests.cpp
    value_tests.cpp

)

target_include_directories(test PRIVATE
    ${GTEST_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/src
)

target_link_directories(test PRIVATE
    ${GTEST_LIBRARY_DIRS}
)

target_link_libraries(test PRIVATE
    ${GTEST_LIBRARIES}
)

if(COVERAGE)
    target_compile_options(test PRIVATE -fprofile-arcs -ftest-coverage -O1 --coverage -fno-inline)
    target_link_libraries(test PRIVATE -lgcov)
endif()

add_custom_target(tests-run
    COMMAND ./test --gtest_color=yes
    DEPENDS test
)

if(COVERAGE)
    add_coverage_targets(test tests-run tests ${PROJECT_SOURCE_DIR}/src)
endif()
