cmake_minimum_required(VERSION 3.26.4)
project(JvmAcquirer)

set(CMAKE_CXX_STANDARD 20)

enable_language( ASM_MASM )


add_compile_options( "/MP" )

include_directories(
        include
        include/break
        include/classes
        include/utility
        include/runtime
        library
        library/jni
        library/detours
        jnif
)

file(GLOB_RECURSE LIBS library/**.lib)
file(GLOB_RECURSE INCLUDES include/**.h include/**.hpp)
file(GLOB_RECURSE SOURCES
        source/**.cpp
        library/**.cpp
        source/**.asm
        library/**.asm
        include/**.asm
        jnif/**.cpp
)

add_library(${PROJECT_NAME} SHARED
        ${SOURCES}
        ${INCLUDES}
)

target_link_libraries(
        ${PROJECT_NAME}
        ${LIBS}
)
