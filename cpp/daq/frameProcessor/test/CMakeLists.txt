
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS} 
	${Boost_INCLUDE_DIR} ${LOG4CXX_INCLUDE_DIR}/.. ${ZEROMQ_INCLUDE_DIRS})

# Build list of test source files from current dir
file(GLOB TEST_SOURCES *.cpp)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -w -O3")


# Add test and project source files to executable
add_executable(percivalFrameProcessorTest PercivalFrameProcessorTest.cpp)

add_executable(percivalCalibTest PercivalCalibTests.cpp)
add_executable(percivalCalibRegressionTest PercivalCalibRTests.cpp)
add_executable(SimdTest SimdTests.cpp)
add_executable(meanvar-gen meanvar-gen.cpp)

target_include_directories(meanvar-gen PRIVATE "${HDF5_ROOT}/include")

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
# librt required for timing functions
find_library(REALTIME_LIBRARY 
		NAMES rt)
target_link_libraries( percivalFrameProcessorTest ${REALTIME_LIBRARY} )
endif()

# Define libraries to link against
target_link_libraries(percivalFrameProcessorTest
	    PercivalProcess2Plugin
		${ODINDATA_LIBRARIES} 
		${Boost_LIBRARIES}
		${LOG4CXX_LIBRARIES}
		${ZEROMQ_LIBRARIES}) 

target_link_libraries(percivalCalibTest
        PercivalCalib
		${Boost_LIBRARIES}
		${LOG4CXX_LIBRARIES}
)

target_link_libraries(percivalCalibRegressionTest
        PercivalCalib
		${Boost_LIBRARIES}
		${LOG4CXX_LIBRARIES}
)

target_link_libraries(SimdTest
		${Boost_LIBRARIES}
)

target_link_libraries(meanvar-gen
    PercivalCalib
		${Boost_LIBRARIES}
)

install(TARGETS meanvar-gen RUNTIME DESTINATION bin)




