
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIR} ${LOG4CXX_INCLUDE_DIR}/.. ${ZEROMQ_INCLUDE_DIRS})

add_library(PercivalProcessPlugin SHARED PercivalProcessPlugin.cpp)
add_library(PercivalProcess2Plugin SHARED PercivalProcess2Plugin.cpp)
add_library(PercivalProcess3Plugin SHARED PercivalProcess3Plugin.cpp)
add_library(PercivalGenPlugin SHARED PercivalGenPlugin.cpp)
add_library(PercivalCalibPlugin SHARED PercivalCalibPlugin.cpp)

# warning this applies to the whole file - can we do better?
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -fpic")
add_library(PercivalCalib STATIC CalibratorSample.cpp CalibratorReset.cpp FrameMem.cpp)
target_include_directories(PercivalCalib PRIVATE "${HDF5_ROOT}/include")

# this lines carries the dependencies forwards to anyone who uses PercivalCalib
target_link_libraries(PercivalCalib PUBLIC libtbb.so ${HDF5_ROOT}/lib/libhdf5.so ${HDF5_ROOT}/lib/libhdf5_hl.so ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES})

target_link_libraries(PercivalCalibPlugin PUBLIC PercivalCalib ${ODINDATA_LIBRARIES})

install(TARGETS PercivalProcessPlugin PercivalProcess2Plugin PercivalProcess3Plugin PercivalGenPlugin PercivalCalibPlugin LIBRARY DESTINATION lib)

# this bit of fun is to help us see what link deps are missing from PercivalCalibPlugin.
# The build system tries to grab what it can for PercivalCalibPlugin, but it may be incomplete.
# it will crash in the fp if there are missing deps
# you may need to put some code into main() to force the linker to do something.
#add_executable(hello main.cpp)
#target_link_libraries(hello PercivalCalibPlugin)

