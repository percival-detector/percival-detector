
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

set(FRAMERECEIVER_MAIN_DIR ${ODINDATA_ROOTDIR}/frameReceiver)
set(FRAMERECEIVER_MAIN_SOURCE_DIR ${FRAMERECEIVER_MAIN_DIR}/src)
set(FRAMERECEIVER_MAIN_INCLUDE_DIR ${FRAMERECEIVER_MAIN_DIR}/include)

include_directories(${FRAMERECEIVER_DIR}/include 
					${FRAMERECEIVER_MAIN_INCLUDE_DIR} 
					${ODINDATA_INCLUDE_DIRS} 
					${Boost_INCLUDE_DIRS} 
					${LOG4CXX_INCLUDE_DIRS}/.. 
					${ZEROMQ_INCLUDE_DIRS})

# Build list of test source files from current dir
file(GLOB TEST_SOURCES *.cpp)

# Build list of main project source files from src dir but exclude application main
file(GLOB APP_SOURCES ${FRAMERECEIVER_MAIN_SOURCE_DIR}/*.cpp)
file(GLOB APP_MAIN_SOURCE ${FRAMERECEIVER_MAIN_SOURCE_DIR}/FrameReceiverApp.cpp)
list(REMOVE_ITEM APP_SOURCES ${APP_MAIN_SOURCE})

# Add test and project source files to executable
add_executable(percivalFrameReceiverTest ${TEST_SOURCES} ${APP_SOURCES})

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
# librt required for timing functions
find_library(REALTIME_LIBRARY 
		NAMES rt)
target_link_libraries( percivalFrameReceiverTest ${REALTIME_LIBRARY} )
endif()

# Define libraries to link against
target_link_libraries(percivalFrameReceiverTest
	    PercivalEmulatorFrameDecoder
		${ODINDATA_LIBRARIES} 
		${Boost_LIBRARIES}
		${LOG4CXX_LIBRARIES}
		${ZEROMQ_LIBRARIES}) 
