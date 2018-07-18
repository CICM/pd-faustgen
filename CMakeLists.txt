cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

include(pd.build/pd.cmake)

project(faustgen~ C)

## On Linux "force" the link with stdc++
if(UNIX AND NOT APPLE)
    set(FAUST_LIBS  "stdc++"  CACHE STRING  "FAUST LIBRARIES" FORCE)
endif()

## For Windows use static runtime instead of default dynamic runtime
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT"  CACHE STRING  "FORCE CXX FLAGS" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "/MT"  CACHE STRING  "FORCE C FLAGS" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd"  CACHE STRING  "FORCE CXX FLAGS" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "/MTd"  CACHE STRING  "FORCE C FLAGS" FORCE)
    set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

include(FaustLib.cmake)
## Create Faust~
message(STATUS "faustgen~ external")

## Create the Pure Data external
set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/external/")
file(GLOB faustgen_tilde_sources
${PROJECT_SOURCE_DIR}/src/faust_tilde.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_ui.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_ui.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_io.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_io.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_options.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_options.c)
add_pd_external(faustgen_tilde_project faustgen~ "${faustgen_tilde_sources}")

## Link the Pure Data external with faustlib
include_directories(${PROJECT_SOURCE_DIR}/faust/architecture)
add_dependencies(faustgen_tilde_project staticlib)
target_link_libraries(faustgen_tilde_project staticlib)

## Link the Pure Data external with llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(llvm_libs all)
list(REMOVE_ITEM llvm_libs LTO)
target_link_libraries(faustgen_tilde_project ${llvm_libs})
if(WIN32)
  target_link_libraries(faustgen_tilde_project ws2_32)
endif()

if(MSVC)
    set_property(TARGET faustgen_tilde_project APPEND_STRING PROPERTY LINK_FLAGS " /ignore:4099 ")
endif()
