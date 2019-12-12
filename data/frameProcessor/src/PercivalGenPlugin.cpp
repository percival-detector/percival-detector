/*
 * PercivalGenPlugin.cpp
 *
 *  Created on: 7 Dec 2019
 *      Author: ulw43618
 */

#include "PercivalGenPlugin.h"
#include "version.h"

#include <FrameMetaData.h>
#include <DataBlockFrame.h>

#include <log4cxx/logger.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/simplelayout.h>

#include <thread>

namespace FrameProcessor
{
    const std::string PercivalGenPlugin::CONFIG_PROCESS             = "process";
    const std::string PercivalGenPlugin::CONFIG_PROCESS_NUMBER      = "number";
    const std::string PercivalGenPlugin::CONFIG_PROCESS_RANK        = "rank";

    PercivalGenPlugin::PercivalGenPlugin() :
    frame_counter_(0),
    concurrent_processes_(1),
    concurrent_rank_(0)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.PercivalGenPlugin");
    logger_->setLevel(Level::getAll());
    log4cxx::LayoutPtr layout(new log4cxx::SimpleLayout());
    FileAppender* appender(new FileAppender(layout, "wlog.txt", false));
    logger_->removeAllAppenders();
    logger_->addAppender(appender);
  //  auto l = logger_->getAllAppenders();

    LOG4CXX_INFO(logger_, "PercivalGenPlugin version " << this->get_version_long() << " loaded");


    mythread_ = std::move(std::thread(&PercivalGenPlugin::threadfn, this));
  }

  PercivalGenPlugin::~PercivalGenPlugin()
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
  void PercivalGenPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Protect this method
    LOG4CXX_INFO(logger_, config.encode());

    // Check to see if we are configuring the process number and rank
    if (config.has_param(PercivalGenPlugin::CONFIG_PROCESS)) {
      OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(PercivalGenPlugin::CONFIG_PROCESS));
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
  void PercivalGenPlugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Check for process number and rank number
    if (config.has_param(PercivalGenPlugin::CONFIG_PROCESS_NUMBER)) {
      this->concurrent_processes_ = config.get_param<size_t>(PercivalGenPlugin::CONFIG_PROCESS_NUMBER);
      LOG4CXX_INFO(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    if (config.has_param(PercivalGenPlugin::CONFIG_PROCESS_RANK)) {
      this->concurrent_rank_ = config.get_param<size_t>(PercivalGenPlugin::CONFIG_PROCESS_RANK);
      LOG4CXX_INFO(logger_, "Process rank changed to " << this->concurrent_rank_);
    }
  }

  bool PercivalGenPlugin::reset_statistics()
  {
    LOG4CXX_INFO(logger_, "PercivalGenPlugin reset_statistics called");
    frame_counter_ = this->concurrent_rank_;
    return true;
  }

  int PercivalGenPlugin::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  int PercivalGenPlugin::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  int PercivalGenPlugin::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

  std::string PercivalGenPlugin::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  std::string PercivalGenPlugin::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

  void PercivalGenPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_ERROR(logger_, "Processing frame in generator - wrong!");
 
  }

  void PercivalGenPlugin::threadfn()
  {
     LOG4CXX_INFO(logger_, "thread fn starting");
     while(true)
     {
        FrameMetaData md;
        dimensions_t dims{FRAME_ROWS, FRAME_COLS};
        md.set_dataset_name("reset");
        md.set_frame_number(frame_counter_++);
        md.set_data_type(raw_16bit);
        md.set_dimensions(dims);
      //  md.set_acquisition_ID("aq1");
        md.set_compression_type(no_compression);
        
        boost::shared_ptr<Frame> resetfr, datafr;
        int sz = dims[0] * dims[1] * sizeof(uint16_t);

        int imgOffset = 0;
        resetfr.reset(new DataBlockFrame(md, sz, imgOffset));
        void* ptr = resetfr->get_data_ptr();
        int extra = (uint64_t)ptr & 0x1f;
        if(extra)
        {
            LOG4CXX_ERROR(logger_, "alignment wrong on reset block " << extra);
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
        LOG4CXX_INFO(logger_, "pushing reset frame");
        this->push(resetfr);

        md.set_dataset_name("data");
        datafr.reset(new DataBlockFrame(md, sz, imgOffset));

        std::this_thread::sleep_for(std::chrono::seconds(10));
        this->push(datafr);
     }
  }

} /* namespace FrameProcessor */

