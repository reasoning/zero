cmake_minimum_required(VERSION 3.0.0)
project(zero VERSION 0.1.0)

# Compile options
add_compile_options(-w)
add_compile_options(-fpermissive -fno-omit-frame-pointer -fmax-errors=1)
#add_compile_options(-std=c++0x)

add_compile_options(-fno-exceptions -fno-rtti)
add_link_options(-nodefaultlibs -lc)

# Debug
add_compile_options(-O0 -ggdb)

# Release
#add_compile_options(-O2 -ggdb)

set(MODULE_NAME ${CMAKE_PROJECT_NAME})

#message(STATUS "PROJECT: ${CMAKE_PROJECT_NAME}")
#message(STATUS "MODULE: ${MODULE_NAME}")


include(FetchContent)
FetchContent_Declare(hero
    GIT_REPOSITORY https://gitlab.com/reasonio/hero.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(hero)

file(GLOB_RECURSE MODULE_SRC_FILES src/*.cpp)
add_executable(${MODULE_NAME} ${MODULE_SRC_FILES})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src )
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/hero-src/src )

target_link_libraries(${MODULE_NAME} libhero)

