/*
 * PercivalCalibPlugin.cpp
 *
 *  Created on: 7 Dec 2019
 *      Author: ulw43618
 */

#include "PercivalCalibPlugin.h"
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
#include <chrono>

using namespace std::chrono;

namespace FrameProcessor
{
    const std::string PercivalCalibPlugin::CONFIG_PROCESS             = "process";
    const std::string PercivalCalibPlugin::CONFIG_PROCESS_NUMBER      = "number";
    const std::string PercivalCalibPlugin::CONFIG_PROCESS_RANK        = "rank";

    PercivalCalibPlugin::PercivalCalibPlugin() :
    frame_counter_(0),
    concurrent_processes_(1),
    concurrent_rank_(0),
    m_calibratorReset(FRAME_ROWS, FRAME_COLS),
    m_calibratorSample(FRAME_ROWS, FRAME_COLS)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.PercivalCalibPlugin");
    logger_->setLevel(Level::getAll());
   // log4cxx::LayoutPtr layout(new log4cxx::SimpleLayout());
  //  FileAppender* appender(new FileAppender(layout, "wlog.txt", false));
  //  logger_->removeAllAppenders();
 //   logger_->addAppender(appender);
  //  auto l = logger_->getAllAppenders();


    LOG4CXX_INFO(logger_, "PercivalCalibPlugin version " << this->get_version_long() << " loaded");

  }

  PercivalCalibPlugin::~PercivalCalibPlugin()
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
  void PercivalCalibPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Protect this method
    LOG4CXX_INFO(logger_, config.encode());

    // Check to see if we are configuring the process number and rank
    if (config.has_param(PercivalCalibPlugin::CONFIG_PROCESS)) {
      OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(PercivalCalibPlugin::CONFIG_PROCESS));
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
  void PercivalCalibPlugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Check for process number and rank number
    if (config.has_param(PercivalCalibPlugin::CONFIG_PROCESS_NUMBER)) {
      this->concurrent_processes_ = config.get_param<size_t>(PercivalCalibPlugin::CONFIG_PROCESS_NUMBER);
      LOG4CXX_INFO(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    if (config.has_param(PercivalCalibPlugin::CONFIG_PROCESS_RANK)) {
      this->concurrent_rank_ = config.get_param<size_t>(PercivalCalibPlugin::CONFIG_PROCESS_RANK);
      LOG4CXX_INFO(logger_, "Process rank changed to " << this->concurrent_rank_);
    }
  }

  bool PercivalCalibPlugin::reset_statistics()
  {
    LOG4CXX_INFO(logger_, "PercivalCalibPlugin reset_statistics called");
    frame_counter_ = this->concurrent_rank_;
    return true;
  }

  int PercivalCalibPlugin::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  int PercivalCalibPlugin::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  int PercivalCalibPlugin::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

  std::string PercivalCalibPlugin::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  std::string PercivalCalibPlugin::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

  void PercivalCalibPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    std::chrono::high_resolution_clock::time_point start = high_resolution_clock::now();

    std::string name = frame->get_meta_data().get_dataset_name();
    LOG4CXX_TRACE(logger_, "Got calib frame " << name);
    if(name == "data")
    {
        boost::shared_ptr<Frame> newfr;
        dimensions_t dims{FRAME_ROWS, FRAME_COLS};

        int sz = dims[0] * dims[1] * sizeof(float);
        newfr.reset(new DataBlockFrame(frame->get_meta_data(), sz));
        newfr->meta_data().set_dataset_name("data");

        MemBlockI16 in;
        MemBlockF out;
        in.init(FRAME_ROWS, FRAME_COLS, frame->get_image_ptr());
        out.init(FRAME_ROWS, FRAME_COLS, newfr->get_image_ptr());

        LOG4CXX_TRACE(logger_, "Processing calib frame");
        m_calibratorSample.processFrameP(in,out);

        this->push(newfr);
    }

    auto end = high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOG4CXX_TRACE(logger_, "Processing took us:" << duration.count());
  }
} /* namespace FrameProcessor */
