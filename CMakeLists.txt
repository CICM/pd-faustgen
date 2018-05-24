cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)
include(pd.build/pd.cmake)

project(faust~ C)

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

if(NOT LIBFAUST_PATH)
    if(WIN32)
        if(${CMAKE_GENERATOR} MATCHES "(Win64|IA64)")
            set(LIBFAUST_PATH  ${PROJECT_SOURCE_DIR}/faust/build/lib64)
        else()
            set(LIBFAUST_PATH  ${PROJECT_SOURCE_DIR}/faust/build/lib32)
        endif()
    else()
        set(LIBFAUST_PATH  ${PROJECT_SOURCE_DIR}/faust/build/lib)
    endif()
endif()

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_library(faust STATIC IMPORTED)
if(MSVC)
  set_target_properties(faust PROPERTIES
  IMPORTED_LOCATION ${LIBFAUST_PATH}/faust.lib
  INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/faust/architecture")
else()
  set_target_properties(faust PROPERTIES
  IMPORTED_LOCATION ${LIBFAUST_PATH}/libfaust.a
  INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/faust/architecture")
endif()
target_link_libraries(faust_tilde_project faust)
llvm_map_components_to_libnames(llvm_libs all)
list(REMOVE_ITEM llvm_libs LTO)
target_link_libraries(faust_tilde_project ${llvm_libs})

if(WIN32)
  target_link_libraries(faust_tilde_project ws2_32)
#  target_link_libraries(faust_tilde_project msvcrt)
#  target_link_libraries(faust_tilde_project oldnames)
endif()
