cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

include(pd.build/pd.cmake)

project(faust~ C)

## For Windows use static runtime instead of default dynamic runtime
if(MSVC)
    set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

include(FaustLib.cmake)
## Create Faust~
message(STATUS "Faust External")

## Create the Pure Data external
set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/external/")
add_pd_external(faust_tilde_project faust~ ${PROJECT_SOURCE_DIR}/src/faust_tilde.c)

## Link the Pure Data external with faustlib
include_directories(${PROJECT_SOURCE_DIR}/faust/architecture)
add_dependencies(faust_tilde_project staticlib)
target_link_libraries(faust_tilde_project staticlib)

if(MSVC)
    set_property(TARGET staticlib APPEND_STRING PROPERTY COMPILE_FLAGS " /EHsc ")
    set_property(TARGET staticlib APPEND_STRING PROPERTY COMPILE_FLAGS " /D WIN32 ")

    add_library(faust_lib STATIC IMPORTED)
    set_target_properties(faust_lib PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/faust/build/lib/faust.lib
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/faust/build/lib/Debug/faust.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/faust/build/lib/Release/faust.lib
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/faust/architecture")
    target_link_libraries(faust_tilde_project faust_lib)
endif()

## Link the Pure Data external with llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(llvm_libs all)
list(REMOVE_ITEM llvm_libs LTO)
target_link_libraries(faust_tilde_project ${llvm_libs})
if(WIN32)
  target_link_libraries(faust_tilde_project ws2_32)
endif()
