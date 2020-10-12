
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

add_library(PercivalProcessPlugin SHARED PercivalProcessPlugin.cpp)
add_library(PercivalProcess2Plugin SHARED PercivalProcess2Plugin.cpp)
add_library(PercivalProcess3Plugin SHARED PercivalProcess3Plugin.cpp)
add_library(PercivalGenPlugin SHARED PercivalGenPlugin.cpp)
add_library(PercivalCalibPlugin SHARED PercivalCalibPlugin.cpp)

# warning this applies to the whole file - can we do better?
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -fpic")
add_library(PercivalCalib STATIC CalibratorSample.cpp CalibratorReset.cpp FrameMem.cpp)
target_include_directories(PercivalCalib PRIVATE "${HDF5_ROOT}/include")

target_link_libraries(PercivalCalib PUBLIC libtbb.so ${HDF5_ROOT}/lib/libhdf5.so ${HDF5_ROOT}/lib/libhdf5_hl.so)

target_link_libraries(PercivalCalibPlugin PercivalCalib)

install(TARGETS PercivalProcessPlugin PercivalProcess2Plugin PercivalProcess3Plugin PercivalGenPlugin PercivalCalibPlugin LIBRARY DESTINATION lib)

# this bit of fun is to help us see what link deps are missing from PercivalCalib
# cos it won't load in the fp if there are missing deps
# target_link_libraries(PercivalCalibPlugin ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES})
# add_executable(hello main.cpp)
# target_link_libraries(hello PercivalCalib)

