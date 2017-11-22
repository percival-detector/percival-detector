/*
 * PercivalProcessPlugin.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#include <PercivalProcessPlugin.h>

namespace FrameProcessor
{

  PercivalProcessPlugin::PercivalProcessPlugin()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FW.PercivalProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "PercivalProcessPlugin constructor.");
  }

  PercivalProcessPlugin::~PercivalProcessPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  void PercivalProcessPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Processing raw frame.");
    // This plugin will take the raw buffer and convert to two percival frames
    // One reset frame and one data frame
    // Assuming this is a P2M, our image dimensions are:
    size_t bytes_per_pixel = 2;
    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;
    dimensions_t p2m_subframe_dims = p2m_dims; p2m_subframe_dims[1] = p2m_subframe_dims[1] >> 1;
    size_t subframe_bytes = p2m_subframe_dims[0] * p2m_subframe_dims[1] * bytes_per_pixel;
    size_t bytes_per_subframe_row = p2m_subframe_dims[1]*bytes_per_pixel;

    // Read out the frame header from the raw frame
    const PercivalEmulator::FrameHeader* hdrPtr = static_cast<const PercivalEmulator::FrameHeader*>(frame->get_data());
    LOG4CXX_TRACE(logger_, "Raw frame number: " << hdrPtr->frame_number);

    // Raw frame arrive as two sub-frames stored incorrectly in memory
    // -------------------
    // |        |        |
    // |        |        |
    // |        |        |
    // |        |        |
    // |        |        |
    // |        |        |
    // -------------------
    //
    // These need to be shuffled into correct continuous memory format
    // Loop over each row, copying the row from sub-frame 1 and then sub-frame 2
    // into the new frame
    void *tmp_mem_ptr = malloc(PercivalEmulator::data_type_size);

    boost::shared_ptr<Frame> reset_frame;
    reset_frame = boost::shared_ptr<Frame>(new Frame("reset"));
    reset_frame->set_frame_number(hdrPtr->frame_number);
    reset_frame->set_dimensions("reset", p2m_dims);
    // Copy data into frame
    // TODO: This is a fudge as the Frame object will not permit write access to the memory
    // TODO: Frame object needs an init method and also write access.
    char *dest_ptr = (char *)tmp_mem_ptr;
    const char *src_ptr = (static_cast<const char*>(frame->get_data())+sizeof(PercivalEmulator::FrameHeader)+PercivalEmulator::data_type_size);
    for (int row = 0; row < p2m_dims[0]; row++){
    	// Copy the first half of the row
    	memcpy(dest_ptr, src_ptr, bytes_per_subframe_row);
    	// Increment the destination pointer by half a row
    	dest_ptr += bytes_per_subframe_row;
    	// Copy the second half of the row
    	memcpy(dest_ptr, src_ptr+subframe_bytes, bytes_per_subframe_row);
    	// Increment the source and destination pointers by half a row
    	src_ptr += bytes_per_subframe_row;
    	dest_ptr += bytes_per_subframe_row;
    }
    // Copy data into frame
    reset_frame->copy_data(tmp_mem_ptr, PercivalEmulator::data_type_size);
    LOG4CXX_TRACE(logger_, "Pushing reset frame.");
    this->push(reset_frame);

    boost::shared_ptr<Frame> data_frame;
    data_frame = boost::shared_ptr<Frame>(new Frame("data"));
    data_frame->set_frame_number(hdrPtr->frame_number);
    data_frame->set_dimensions("data", p2m_dims);
    // Copy data into frame
    // TODO: This is a fudge as the Frame object will not permit write access to the memory
    // TODO: Frame object needs an init method and also write access.
    dest_ptr = (char *)tmp_mem_ptr;
    src_ptr = (static_cast<const char*>(frame->get_data())+sizeof(PercivalEmulator::FrameHeader));
    for (int row = 0; row < p2m_dims[0]; row++){
    	// Copy the first half of the row
    	memcpy(dest_ptr, src_ptr, bytes_per_subframe_row);
    	// Increment the destination pointer by half a row
    	dest_ptr += bytes_per_subframe_row;
    	// Copy the second half of the row
    	memcpy(dest_ptr, src_ptr+subframe_bytes, bytes_per_subframe_row);
    	// Increment the source and destination pointers by half a row
    	src_ptr += bytes_per_subframe_row;
    	dest_ptr += bytes_per_subframe_row;
    }
    // Copy data into frame
    data_frame->copy_data(tmp_mem_ptr, PercivalEmulator::data_type_size);
    LOG4CXX_TRACE(logger_, "Pushing data frame.");
    this->push(data_frame);

    // Free temporary memory allocation
    free(tmp_mem_ptr);

  }

} /* namespace FrameProcessor */
