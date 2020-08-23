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
## faustgen~, in which case compilation will fail. If you plan to upload the
## external to Deken, we recommend leaving this option OFF, since that will
## gurantee that libfaust is linked statically into the external.
set(INSTALLED_FAUST "OFF"  CACHE BOOL  "Use an installed Faust library")

## In addition, set this to OFF in order to link to the shared libfaust
## library. Default is static linking. Note that this option only has an
## effect if INSTALLED_FAUST is ON. Static linking is always used when linking
## with the included Faust version.
set(STATIC_FAUST "ON"  CACHE BOOL  "Link the installed Faust library statically if possible")

message(STATUS "Installed Faust library: ${INSTALLED_FAUST}")
if(INSTALLED_FAUST)
message(STATUS "Installed Faust static linking: ${STATIC_FAUST}")
else()
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
  if(STATIC_FAUST)
    if(MSVC)
      find_library(FAUST_LIBRARY faust.lib REQUIRED)
    else()
      find_library(FAUST_LIBRARY libfaust.a REQUIRED)
    endif()
  else()
    if(MSVC)
      find_library(FAUST_LIBRARY faust.dll REQUIRED)
    else()
      find_library(FAUST_LIBRARY NAMES libfaust.so libfaust.dylib REQUIRED)
    endif()
  endif()
  # Double-check that the file actually exists, in case the user specified a
  # wrong path.
  if(FAUST_LIBRARY AND EXISTS ${FAUST_LIBRARY})
    ## Based on FAUST_LIBRARY we can make an educated guess about the
    ## locations of the Faust include and library directories for the specific
    ## Faust installation we're using. These should work in most cases, but we
    ## also do a more general search in standard locations as a fallback.
    get_filename_component(FAUST_LIBRARY_DIR ${FAUST_LIBRARY} DIRECTORY)
    if(FAUST_LIBRARY_DIR)
      find_path(FAUST_INCLUDE_DIR dsp/llvm-c-dsp.h HINTS "${FAUST_LIBRARY_DIR}/../include/faust" NO_DEFAULT_PATH)
      if(NOT FAUST_INCLUDE_DIR)
	find_path(FAUST_INCLUDE_DIR dsp/llvm-c-dsp.h PATH_SUFFIXES faust include/faust REQUIRED)
      endif()
      find_path(FAUSTLIB all.lib HINTS "${FAUST_LIBRARY_DIR}/../share/faust" NO_DEFAULT_PATH)
      if(NOT FAUSTLIB)
	find_path(FAUSTLIB all.lib PATH_SUFFIXES faust share/faust REQUIRED)
      endif()
    endif()
    message(STATUS "Found installed Faust library at: ${FAUST_LIBRARY}")
    if(FAUST_INCLUDE_DIR AND EXISTS "${FAUST_INCLUDE_DIR}/dsp/llvm-c-dsp.h")
      message(STATUS "Found installed Faust include files at: ${FAUST_INCLUDE_DIR}")
    else()
      message(FATAL_ERROR "Faust include files not found, maybe you specified the wrong FAUST_INCLUDE_DIR directory? Otherwise try using the included Faust instead (INSTALLED_FAUST=OFF).")
    endif()
    if(FAUSTLIB AND EXISTS "${FAUSTLIB}/all.lib")
      message(STATUS "Found installed Faust library files at: ${FAUSTLIB}")
    else()
      message(FATAL_ERROR "Faust library files not found, maybe you specified the wrong FAUSTLIB directory? Otherwise try using the included Faust instead (INSTALLED_FAUST=OFF).")
    endif()
  else()
    message(FATAL_ERROR "Faust library not found, maybe you specified the wrong FAUST_LIBRARY directory? Otherwise try using the included Faust instead (INSTALLED_FAUST=OFF).")
  endif()
  include_directories(${FAUST_INCLUDE_DIR})
  target_link_libraries(faustgen_tilde_project ${FAUST_LIBRARY})
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
if(llvm_libs)
  list(REMOVE_ITEM llvm_libs LTO)
endif()
## Work around llvm_map_components_to_libnames producing an empty result at
## least with some LLVM versions. In such a case, llvm-config can hopefully
## provide us with the correct options. Note that this requires that
## llvm-config can be found on PATH.
if(NOT llvm_libs)
  execute_process(COMMAND llvm-config --libs OUTPUT_VARIABLE llvm_config_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT llvm_config_libs)
    message(WARNING "Tried to get LLVM libraries from both cmake and llvm-config, but both came up empty (maybe try to set llvm_libs manually)")
  else()
    message(STATUS "Using fallback LLVM linker options (llvm-config --libs): ${llvm_config_libs}")
    # Make sure that we get a proper cmake list in case llvm-config returned
    # multiple libraries.
    string(REPLACE " " ";" llvm_libs "${llvm_config_libs}")
  endif()
endif()
target_link_libraries(faustgen_tilde_project ${llvm_libs})
if(WIN32)
  target_link_libraries(faustgen_tilde_project ws2_32)
endif()

if(MSVC)
    set_property(TARGET faustgen_tilde_project APPEND_STRING PROPERTY LINK_FLAGS " /ignore:4099 ")
endif()

## Installation directory. This is relative to CMAKE_INSTALL_PREFIX.
## Default is lib/pd/extra/faustgen~ on Linux and other generic Unix-like
## systems, or just faustgen~ on Mac and Windows.
if(UNIX AND NOT APPLE)
    set(INSTALL_DIR "lib/pd/extra/faustgen~" CACHE STRING "Destination directory for the external")
else()
    set(INSTALL_DIR "faustgen~" CACHE STRING "Destination directory for the external")
endif()

message(STATUS "Installation goes to CMAKE_INSTALL_PREFIX/${INSTALL_DIR}")
message(STATUS "(set the INSTALL_DIR variable to override)")

if(INSTALLED_FAUST)
  ## Grab the .lib files from the installed Faust using the FAUSTLIB path.
  file(GLOB lib_files ${FAUSTLIB}/*.lib)
else()
  file(GLOB lib_files ${PROJECT_SOURCE_DIR}/faust/libraries/*.lib ${PROJECT_SOURCE_DIR}/faust/libraries/old/*.lib)
endif()
install(DIRECTORY external/ DESTINATION ${INSTALL_DIR})
install(FILES ${lib_files} DESTINATION ${INSTALL_DIR}/libs)
