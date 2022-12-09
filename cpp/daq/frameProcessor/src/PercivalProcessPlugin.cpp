/*
 * PercivalProcessPlugin.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#include <PercivalProcessPlugin.h>
#include <DataBlockFrame.h>
#include "percival_version.h"

#include <limits>

static constexpr int64_t UNSET = std::numeric_limits<int64_t>::max();

namespace FrameProcessor
{
    const std::string PercivalProcessPlugin::CONFIG_PROCESS             = "process";
    const std::string PercivalProcessPlugin::CONFIG_PROCESS_NUMBER      = "number";
    const std::string PercivalProcessPlugin::CONFIG_PROCESS_RANK        = "rank";

    PercivalProcessPlugin::PercivalProcessPlugin() :
    frame_base_(UNSET),
    concurrent_processes_(1),
    concurrent_rank_(0)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.PercivalProcessPlugin");
    LOG4CXX_INFO(logger_, "PercivalProcessPlugin version " << this->get_version_long() << " loaded");
  }

  PercivalProcessPlugin::~PercivalProcessPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  /**
   * Set configuration options for the Percival processing plugin.
   *
   * This sets up the process plugin according to the configuration IpcMessage
   * objects that are received. The options are searched for:
   * CONFIG_PROCESS - Calls the method processConfig
   *
   * \param[in] config - IpcMessage containing configuration data.
   * \param[out] reply - Response IpcMessage.
   */
  void PercivalProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Protect this method
    LOG4CXX_INFO(logger_, config.encode());

    // Check to see if we are configuring the process number and rank
    if (config.has_param(PercivalProcessPlugin::CONFIG_PROCESS)) {
      OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(PercivalProcessPlugin::CONFIG_PROCESS));
      this->configureProcess(processConfig, reply);
    }
  }

  /**
   * Set configuration options for the Percival process count.
   *
   * This sets up the process plugin according to the configuration IpcMessage
   * objects that are received. The options are searched for:
   * CONFIG_PROCESS_NUMBER - Sets the number of writer processes executing
   * CONFIG_PROCESS_RANK - Sets the rank of this process
   *
   * \param[in] config - IpcMessage containing configuration data.
   * \param[out] reply - Response IpcMessage.
   */
  void PercivalProcessPlugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Check for process number and rank number
    if (config.has_param(PercivalProcessPlugin::CONFIG_PROCESS_NUMBER)) {
      this->concurrent_processes_ = config.get_param<size_t>(PercivalProcessPlugin::CONFIG_PROCESS_NUMBER);
      LOG4CXX_INFO(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    if (config.has_param(PercivalProcessPlugin::CONFIG_PROCESS_RANK)) {
      this->concurrent_rank_ = config.get_param<size_t>(PercivalProcessPlugin::CONFIG_PROCESS_RANK);
      LOG4CXX_INFO(logger_, "Process rank changed to " << this->concurrent_rank_);
    }
  }

  bool PercivalProcessPlugin::reset_statistics()
  {
    LOG4CXX_INFO(logger_, "PercivalProcessPlugin reset_statistics called");
    frame_base_ = UNSET;
    return true;
  }

  int PercivalProcessPlugin::get_version_major()
  {
    return PERCIVAL_VERSION_MAJOR;
  }

  int PercivalProcessPlugin::get_version_minor()
  {
    return PERCIVAL_VERSION_MINOR;
  }

  int PercivalProcessPlugin::get_version_patch()
  {
    return PERCIVAL_VERSION_PATCH;
  }

  std::string PercivalProcessPlugin::get_version_short()
  {
    return PERCIVAL_VERSION_STR_SHORT;
  }

  std::string PercivalProcessPlugin::get_version_long()
  {
    return PERCIVAL_VERSION_STR;
  }

  //! This gets the info field from the buffer header and creates
  // a data frame for it. Private function, not called by framework.
  // you should set the frame number in the md before calling the function
  // @param md a copy of the metadata
  void PercivalProcessPlugin::processInfoField(const PercivalTransport::FrameHeader* hdrPtr, FrameMetaData md)
  {  
    dimensions_t info_dims{1, PercivalTransport::frame_info_size};
    md.set_dataset_name("info");
    md.set_dimensions(info_dims);
    md.set_data_type(FrameProcessor::raw_8bit);

    boost::shared_ptr<Frame> info_frame;
    info_frame.reset(new DataBlockFrame(md, PercivalTransport::frame_info_size));
    char* dest_ptr;
    const char *src_ptr;
    dest_ptr = (char *)info_frame->get_data_ptr();
    src_ptr = (char const*)hdrPtr->frame_info;
    memcpy(dest_ptr, src_ptr, PercivalTransport::frame_info_size);

    LOG4CXX_TRACE(logger_, "Pushing info frame.");
    this->push(info_frame);
  }

  //! This gets the original-framenum from the buffer header and creates
  // a data frame for it. Private function, not called by framework.
  // you should set the frame number in the md before calling the function
  // @param md a copy of the metadata
  void PercivalProcessPlugin::addFrameNumField(const PercivalTransport::FrameHeader* hdrPtr, FrameMetaData md)
  {
    dimensions_t fn_dims{1};
    md.set_dataset_name("original_framenum");
    md.set_dimensions(fn_dims);
    md.set_data_type(FrameProcessor::raw_32bit);

    boost::shared_ptr<Frame> fn_frame;
    fn_frame.reset(new DataBlockFrame(md, 4));
    char* dest_ptr;
    const char *src_ptr;
    dest_ptr = (char *)fn_frame->get_data_ptr();
    src_ptr = (char const*)&hdrPtr->frame_number;
    memcpy(dest_ptr, src_ptr, 4);

    LOG4CXX_TRACE(logger_, "Pushing original_framenum frame.");
    this->push(fn_frame);
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
    const PercivalTransport::FrameHeader* hdrPtr = static_cast<const PercivalTransport::FrameHeader*>(frame->get_data_ptr());
    if(frame_base_==UNSET)
    {
        // we need hdrPtr->frame_number + frame_base_ == concurrent_rank_ for this first frame
        // see Acquisition::adjust_frame_offset()
        frame_base_ = (int64_t)concurrent_rank_ - (int64_t)hdrPtr->frame_number;
        LOG4CXX_INFO(logger_, "first frame after resetst seen, frame_base set to: " << frame_base_);
    }

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
    char* dest_ptr;
    const char *src_ptr;
    FrameMetaData md = frame->meta_data();
    md.set_frame_number(hdrPtr->frame_number);
    md.set_frame_offset(frame_base_);

    processInfoField(hdrPtr, md);
    addFrameNumField(hdrPtr, md);

    md.set_dataset_name("reset");
    md.set_dimensions(p2m_dims);
    md.set_data_type(FrameProcessor::raw_16bit);
    boost::shared_ptr<Frame> reset_frame;
    reset_frame.reset(new DataBlockFrame(md, PercivalTransport::data_type_size));

    // Copy data into frame
    // TODO: This is a fudge as the Frame object will not permit write access to the memory
    // TODO: Frame object needs an init method and also write access.
    dest_ptr = (char *)reset_frame->get_data_ptr();
    // can we use get_image_ptr() here?
    src_ptr = (static_cast<const char*>(frame->get_data_ptr())+sizeof(PercivalTransport::FrameHeader)+PercivalTransport::data_type_size);
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

    LOG4CXX_TRACE(logger_, "Pushing reset frame.");
    this->push(reset_frame);

    md.set_dataset_name("data");
    boost::shared_ptr<Frame> data_frame;
    data_frame.reset(new DataBlockFrame(md, PercivalTransport::data_type_size));

    // Copy data into frame
    // TODO: This is a fudge as the Frame object will not permit write access to the memory
    // TODO: Frame object needs an init method and also write access.
    dest_ptr = (char *)data_frame->get_data_ptr();
    src_ptr = (static_cast<const char*>(frame->get_data_ptr())+sizeof(PercivalTransport::FrameHeader));
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

    // the data frame is conventionally last, and setting this master_dataset on the FW
    // tells it when to move on.
    LOG4CXX_TRACE(logger_, "Pushing data frame last.");
    this->push(data_frame);

    // To configure the FileWriter to save these frames, you will need some json terms like this:
    //   "reset": {
    //      "datatype": "uint16",
    //      "dims": [1484, 1408],
    //      "chunks": [1, 1484, 1408]
    //    },
    //    "info": {
    //      "datatype": "uint8",
    //      "dims": [1,42],
    //      "chunks": [1, 1, 42]
    //    }  
    
  }

} /* namespace FrameProcessor */
