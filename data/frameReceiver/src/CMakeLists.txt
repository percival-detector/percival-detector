
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMERECEIVER_DIR}/include ${ODINDATA_INCLUDE_DIRS} 
	${Boost_INCLUDE_DIR} ${LOG4CXX_INCLUDE_DIR}/.. ${ZEROMQ_INCLUDE_DIRS})

# Add library for Percival frame decoder
add_library(PercivalFrameDecoder SHARED PercivalFrameDecoder.cpp PercivalFrameDecoderLib.cpp)

install(TARGETS PercivalFrameDecoder LIBRARY DESTINATION lib)
