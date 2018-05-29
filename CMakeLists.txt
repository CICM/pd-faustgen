cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

include(pd.build/pd.cmake)
include(ExternalProject)

project(faust~ C)

set(INCLUDE_STATIC  ON)
set(INCLUDE_OSC     OFF)
set(INCLUDE_HTTP    OFF)
set(USE_LLVM_CONFIG OFF)
set(LLVM_DIR_TEMP   ${LLVM_DIR})
set(LLVM_DIR        "./../.${LLVM_DIR_TEMP}")

set(ASMJS_BACKEND  OFF                            CACHE STRING  "Include ASMJS backend" FORCE)
set(C_BACKEND      OFF                            CACHE STRING  "Include C backend"         FORCE)
set(CPP_BACKEND    OFF                            CACHE STRING  "Include CPP backend"       FORCE)
set(FIR_BACKEND    OFF                            CACHE STRING  "Include FIR backend"       FORCE)
set(INTERP_BACKEND OFF                            CACHE STRING  "Include INTERPRETER backend" FORCE)
set(JAVA_BACKEND   OFF                            CACHE STRING  "Include JAVA backend"      FORCE)
set(JS_BACKEND     OFF                            CACHE STRING  "Include JAVASCRIPT backend" FORCE)
set(LLVM_BACKEND   COMPILER STATIC DYNAMIC        CACHE STRING  "Include LLVM backend"      FORCE)
set(OLDCPP_BACKEND OFF                            CACHE STRING  "Include old CPP backend"   FORCE)
set(RUST_BACKEND   OFF                            CACHE STRING  "Include RUST backend"      FORCE)
set(WASM_BACKEND   OFF                            CACHE STRING  "Include WASM backend"  FORCE)

add_subdirectory(./faust/build EXCLUDE_FROM_ALL)
set(LLVM_DIR ${LLVM_DIR_TEMP})

if(MSVC)
    set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/external/")
add_pd_external(faust_tilde_project faust~ ${PROJECT_SOURCE_DIR}/src/faust_tilde.c)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/faust/architecture)
target_link_libraries(faust_tilde_project staticlib)
llvm_map_components_to_libnames(llvm_libs all)
list(REMOVE_ITEM llvm_libs LTO)
target_link_libraries(faust_tilde_project ${llvm_libs})
if(WIN32)
  target_link_libraries(faust_tilde_project ws2_32)
endif()
