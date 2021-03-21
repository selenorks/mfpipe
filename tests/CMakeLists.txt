project(mfpipe_test)

set(SOURCES
        unittest_mfpipe.cpp
)

add_executable(
        mfpipe_test
        ${SOURCES}
        ${HEADERS}
)

target_link_libraries(
        mfpipe_test
        MFPipe
        gtest_main
)

project(mfpipe_gtest)

set(GTEST_SOURCES
        gtest_mfpipe.cpp
)

add_executable(
        mfpipe_gtest
        ${GTEST_SOURCES}
)

target_link_libraries(
        mfpipe_gtest
        MFPipe
        gtest_main
)

add_test(NAME MfPipeGtest COMMAND $<TARGET_FILE:mfpipe_gtest>)
add_test(NAME MfPipeTestTask COMMAND $<TARGET_FILE:mfpipe_test>)