cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

include(pd.build/pd.cmake)

project(faust~ C)
set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/external/")
add_pd_external(faust_tilde_project faust~ ${PROJECT_SOURCE_DIR}/src/faust_tilde.c)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")


include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_library(faust STATIC IMPORTED)
if(MSVC)
  set_target_properties(faust PROPERTIES
  IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/faust/build/lib/faust.lib
  INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/faust/architecture")
else()
  set_target_properties(faust PROPERTIES
  IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/faust/build/lib/libfaust.a
  INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/faust/architecture")
endif()
target_link_libraries(faust_tilde_project faust)
llvm_map_components_to_libnames(llvm_libs all)
list(REMOVE_ITEM llvm_libs LTO)
target_link_libraries(faust_tilde_project ${llvm_libs})
if(WIN32)
  target_link_libraries(faust_tilde_project ws2_32)
endif()
