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

## Set this to ON to link against an installed libfaust rather than the
## version we include. CAVEATS: Use at your own risk. The Faust version
## provided by your system may be too old or too new to be used with
## faustgen~. You also have to make sure that Faust's include files and
## libfaust.so and/or libfaust.a are on the standard include and library
## paths, respectively, otherwise the compilation will fail. If you plan to
## upload the external to Deken, we recommend leaving this option OFF, since
## that will gurantee that libfaust is linked statically into the external.
set(INSTALLED_FAUST "OFF"  CACHE BOOL  "Use an installed Faust library")

message(STATUS "Installed Faust library: ${INSTALLED_FAUST}")
if(NOT INSTALLED_FAUST)
include(FaustLib.cmake)
endif()

## Create Faust~
message(STATUS "faustgen~ external")

## Create the Pure Data external
set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/external/")
file(GLOB faustgen_tilde_sources
${PROJECT_SOURCE_DIR}/src/faustgen_tilde.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_ui.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_ui.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_io.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_io.c
${PROJECT_SOURCE_DIR}/src/faust_tilde_options.h
${PROJECT_SOURCE_DIR}/src/faust_tilde_options.c)
add_pd_external(faustgen_tilde_project faustgen~ "${faustgen_tilde_sources}")

## Link the Pure Data external with faustlib
if(INSTALLED_FAUST)
  target_link_libraries(faustgen_tilde_project "-lfaust")
else()
  include_directories(${PROJECT_SOURCE_DIR}/faust/architecture)
  add_dependencies(faustgen_tilde_project staticlib)
  target_link_libraries(faustgen_tilde_project staticlib)
endif()

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

file(GLOB lib_files ${PROJECT_SOURCE_DIR}/faust/libraries/*.lib ${PROJECT_SOURCE_DIR}/faust/libraries/old/*.lib)
install(DIRECTORY external/ DESTINATION lib/pd/extra/faustgen~)
install(FILES ${lib_files} DESTINATION lib/pd/extra/faustgen~/libs)
