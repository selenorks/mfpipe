cmake_minimum_required(VERSION 3.10)

project(MFPipe)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB
        SOURCES
        internal/*.cpp
        internal/*.h
        internal/protocols/*.h
        internal/protocols/*.h
        internal/protocols/*.cpp
        *.cpp
        *.h
)

add_library(
        ${PROJECT_NAME}
        STATIC
        ${SOURCES}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif(WIN32)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        .
        internal/protocols/${SYSTEM_PREFIX}
)

target_link_libraries(
        ${PROJECT_NAME}
        INTERFACE
        Threads::Threads
)
