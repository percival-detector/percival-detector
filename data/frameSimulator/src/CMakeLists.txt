

ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

file(GLOB LIB_SOURCES PercivalFrameSimulatorPlugin.cpp)

# Add library for excalibur plugin
add_library(PercivalFrameSimulatorPlugin SHARED ${LIB_SOURCES})

target_include_directories(PercivalFrameSimulatorPlugin PRIVATE 
    ${FRAMESIMULATOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

install(TARGETS PercivalFrameSimulatorPlugin LIBRARY DESTINATION lib)
