cmake_minimum_required(VERSION 3.10)

project(MFPipe)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB
        SOURCES
        *.cpp
        *.h
        internal/*.h
        internal/*.cpp
)


add_library(
        ${PROJECT_NAME}
        STATIC
        ${SOURCES}
)


target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        .
)

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        .
)

target_link_libraries(
        ${PROJECT_NAME}
        INTERFACE
        Threads::Threads
)
