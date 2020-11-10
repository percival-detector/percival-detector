

#include <PercivalProcess2Plugin.h>
#include <DataBlockFrame.h>
#include "percival_version.h"

namespace FrameProcessor
{
    const std::string PercivalProcess2Plugin::CONFIG_PROCESS             = "process";
    const std::string PercivalProcess2Plugin::CONFIG_PROCESS_NUMBER      = "number";
    const std::string PercivalProcess2Plugin::CONFIG_PROCESS_RANK        = "rank";

    PercivalProcess2Plugin::PercivalProcess2Plugin() :
    frame_counter_(0),
    concurrent_processes_(1),
    concurrent_rank_(0)
  {
    logger_ = Logger::getLogger("FP.PercivalProcess2Plugin");
    LOG4CXX_INFO(logger_, "PercivalProcess2Plugin version " << this->get_version_long() << " loaded");
  }

  PercivalProcess2Plugin::~PercivalProcess2Plugin()
  {
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
  void PercivalProcess2Plugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Protect this method
    LOG4CXX_INFO(logger_, config.encode());

    // Check to see if we are configuring the process number and rank
    if (config.has_param(PercivalProcess2Plugin::CONFIG_PROCESS)) {
      OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(PercivalProcess2Plugin::CONFIG_PROCESS));
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
  void PercivalProcess2Plugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Check for process number and rank number
    if (config.has_param(PercivalProcess2Plugin::CONFIG_PROCESS_NUMBER)) {
      this->concurrent_processes_ = config.get_param<size_t>(PercivalProcess2Plugin::CONFIG_PROCESS_NUMBER);
      LOG4CXX_INFO(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    if (config.has_param(PercivalProcess2Plugin::CONFIG_PROCESS_RANK)) {
      this->concurrent_rank_ = config.get_param<size_t>(PercivalProcess2Plugin::CONFIG_PROCESS_RANK);
      LOG4CXX_INFO(logger_, "Process rank changed to " << this->concurrent_rank_);
    }
  }

  bool PercivalProcess2Plugin::reset_statistics()
  {
    LOG4CXX_INFO(logger_, "PercivalProcess2Plugin reset_statistics called");
    frame_counter_ = this->concurrent_rank_;
    return true;
  }

  int PercivalProcess2Plugin::get_version_major()
  {
    return PERCIVAL_VERSION_MAJOR;
  }

  int PercivalProcess2Plugin::get_version_minor()
  {
    return PERCIVAL_VERSION_MINOR;
  }

  int PercivalProcess2Plugin::get_version_patch()
  {
    return PERCIVAL_VERSION_PATCH;
  }

  std::string PercivalProcess2Plugin::get_version_short()
  {
    return PERCIVAL_VERSION_STR_SHORT;
  }

  std::string PercivalProcess2Plugin::get_version_long()
  {
    return PERCIVAL_VERSION_STR;
  }

  //! This gets the info field from the buffer header and creates
  // a data frame for it. Private function, not called by framework.
  // you should set the frame number in the md before calling the function
  // @param md a copy of the metadata
  void PercivalProcess2Plugin::processInfoField(const PercivalEmulator::FrameHeader* hdrPtr, FrameMetaData md)
  {  
    dimensions_t info_dims{1, PercivalEmulator::frame_info_size};
    md.set_dataset_name("info");
    md.set_dimensions(info_dims);
    md.set_data_type(FrameProcessor::raw_8bit);

    boost::shared_ptr<Frame> info_frame;
    info_frame.reset(new DataBlockFrame(md, PercivalEmulator::frame_info_size));
    char* dest_ptr;
    const char *src_ptr;
    dest_ptr = (char *)info_frame->get_data_ptr();
    src_ptr = (char const*)hdrPtr->frame_info;
    memcpy(dest_ptr, src_ptr, PercivalEmulator::frame_info_size);

    LOG4CXX_TRACE(logger_, "Pushing info frame.");
    this->push(info_frame);
  }

  void PercivalProcess2Plugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Processing raw frame.");

    // Read out the frame header from the raw frame
    const PercivalEmulator::FrameHeader* hdrPtr = static_cast<const PercivalEmulator::FrameHeader*>(frame->get_data_ptr());
    LOG4CXX_TRACE(logger_, "Raw frame number: " << hdrPtr->frame_number << " offset frame number: " << frame_counter_);

    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;

    FrameMetaData md = frame->meta_data();
    md.set_frame_number(frame_counter_);
    processInfoField(hdrPtr, md);
    
    md.set_dimensions(p2m_dims);
    md.set_data_type(FrameProcessor::raw_16bit);
    md.set_dataset_name("data");
    boost::shared_ptr<Frame> data_frame;
    data_frame.reset(new DataBlockFrame(md, PercivalEmulator::data_type_size));

    // this needs to be incorporated into the descrambler board
    char *dest_ptr = (char *)data_frame->get_data_ptr();
    const char *src_ptr = (static_cast<const char*>(frame->get_data_ptr())+sizeof(PercivalEmulator::FrameHeader)+PercivalEmulator::data_type_size);

    uint32_t bpp = 2;
    uint32_t step_pos = p2m_dims[1] / 4 * 7;
    uint32_t half_frame = p2m_dims[0] * p2m_dims[1] / 2;
    
    for(uint32_t offset = 0; offset < half_frame; offset += step_pos)
    {
        memcpy(dest_ptr, src_ptr, step_pos * bpp);
        dest_ptr += step_pos * bpp;
        memcpy(dest_ptr, src_ptr + half_frame*bpp, step_pos * bpp);
        dest_ptr += step_pos * bpp;
        src_ptr += step_pos * bpp;
    }
    LOG4CXX_TRACE(logger_, "Pushing data frame.");
    this->push(data_frame);

    md.set_dataset_name("reset");
    boost::shared_ptr<Frame> reset_frame;
    reset_frame.reset(new DataBlockFrame(md, PercivalEmulator::data_type_size));

    dest_ptr = (char *)reset_frame->get_data_ptr();
    // can we use get_image_ptr() here?
    src_ptr = (static_cast<const char*>(frame->get_data_ptr())+sizeof(PercivalEmulator::FrameHeader));

    for(uint32_t offset = 0; offset < half_frame; offset += step_pos)
    {
        memcpy(dest_ptr, src_ptr, step_pos * bpp);
        dest_ptr += step_pos * bpp;
        memcpy(dest_ptr, src_ptr + half_frame*bpp, step_pos * bpp);
        dest_ptr += step_pos * bpp;
        src_ptr += step_pos * bpp;
    }

    LOG4CXX_TRACE(logger_, "Pushing reset frame.");
    this->push(reset_frame);

    // Increment local frame counter
    frame_counter_ += this->concurrent_processes_;
  }

} /* namespace FrameProcessor */
