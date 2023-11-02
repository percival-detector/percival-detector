# we need a decent version of cmake for find_library calls
cmake_minimum_required(VERSION 3.7.2)

# Project name
project(PERCIVAL)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-Og -std=c++11 ${CMAKE_CXX_FLAGS}")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIG_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_config)

# Set the common directory
set(COMMON_DIR ${PERCIVAL_SOURCE_DIR}/common)

# Set the frameReceiver directory
set(FRAMERECEIVER_DIR ${PERCIVAL_SOURCE_DIR}/frameReceiver)

# Set the frameProcessor directory
set(FRAMEPROCESSOR_DIR ${PERCIVAL_SOURCE_DIR}/frameProcessor)

set(FRAMESIMULATOR_DIR ${PERCIVAL_SOURCE_DIR}/frameSimulator)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${PERCIVAL_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Allow macOS shared (dynamic) libraries to link with undefined symbols
if (APPLE)
	message("Allowing undefined symbols in macOS shared library linking")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif(APPLE)

# Find and add external packages required for application and test
find_package( Boost 1.41.0
	      REQUIRED
	      COMPONENTS program_options system filesystem unit_test_framework date_time thread)
find_package(Log4CXX 0.10.0 REQUIRED)
find_package(ZeroMQ 3.2.4 REQUIRED)
find_package(OdinData REQUIRED)

message("Determining percival-detector version")
include(GetGitRevisionDescription)
git_describe(GIT_DESC_STR)

string(REGEX REPLACE "^([0-9]+)[\\.-].*" "\\1" VERSION_MAJOR "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-]([0-9]+).*" "\\1" VERSION_MINOR "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-][0-9]+[\\.-]([0-9]+).*" "\\1" VERSION_PATCH "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-][0-9]+[\\.-][0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${GIT_DESC_STR}")

set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(VERSION_STR ${GIT_DESC_STR})
message("-- Git describe version: ${GIT_DESC_STR}")
message("-- major:${VERSION_MAJOR} minor:${VERSION_MINOR} patch:${VERSION_PATCH} sha1:${VERSION_SHA1}")
message("-- short version: ${VERSION_SHORT}")

configure_file(${COMMON_DIR}/include/version.h.in "${CMAKE_BINARY_DIR}/include/percival_version.h")
configure_file(${COMMON_DIR}/include/percival.spec.in "${CMAKE_BINARY_DIR}/include/percival.spec" @ONLY)
include_directories(${CMAKE_BINARY_DIR}/include)

add_custom_target(rpm-package COMMAND rpmbuild -ba ${CMAKE_BINARY_DIR}/include/percival.spec COMMENT "Building RPM - no dependency checks")

# Add common/include directory to include path
include_directories(${COMMON_DIR}/include)

# Add the frameReceiver subdirectory
add_subdirectory(${FRAMERECEIVER_DIR})

# Add the frameReceiver subdirectory
add_subdirectory(${FRAMEPROCESSOR_DIR})

add_subdirectory(frameSimulator)

# Add the doc subdirectory
#add_subdirectory(doc)

# Add the config subdirectory (config files used for integration testing)
#add_subdirectory(config)

