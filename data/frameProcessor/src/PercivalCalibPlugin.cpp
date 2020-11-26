/*
 * PercivalCalibPlugin.cpp
 *
 *  Created on: 7 Dec 2019
 *      Author: ulw43618
 */

#include "PercivalCalibPlugin.h"
#include "percival_version.h"

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

#include <unistd.h>

using namespace std::chrono;

namespace FrameProcessor
{
    // this says where to get the calibration constants from. There should be one file,
    // and all the constants are 64b-doubles.
    const std::string CONFIG_CONSTANTSFILE             = "constantsfile";
    // This value c, if set, says that cma avg will be taken from cols [c,c+31]
    // in the frame of dims 1484r x 1408
    // if it is missing initially, or set to -1 then cma averaging will not happen.
    const std::string CONFIG_CMACOL                    = "cmacol";

    PercivalCalibPlugin::PercivalCalibPlugin() :
    frame_counter_(0),
    concurrent_processes_(1),
    concurrent_rank_(0),
    m_loadedConstants(false),
    m_calibratorReset(FRAME_ROWS, FRAME_COLS),
    m_calibratorSample(FRAME_ROWS, FRAME_COLS)
  {
    logger_ = Logger::getLogger("FP.PercivalCalibPlugin");

    LOG4CXX_INFO(logger_, "PercivalCalibPlugin version " << this->get_version_long() << " loaded");
  }

  PercivalCalibPlugin::~PercivalCalibPlugin()
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
  void PercivalCalibPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    // Protect this method
    LOG4CXX_DEBUG(logger_, config.encode());
    int64_t rc;

    if (config.has_param(CONFIG_CMACOL))
    {
        int cmacol(config.get_param<int>(CONFIG_CMACOL));
        if(0<=cmacol && cmacol < FRAME_COLS)
        {
            LOG4CXX_INFO(logger_, "cma on; col:" << cmacol);
            m_calibratorSample.setCMA(true, cmacol);
        }
        else
        {
            LOG4CXX_INFO(logger_, "cma off");
            m_calibratorSample.setCMA(false,0);
        }
    }

    if (config.has_param(CONFIG_CONSTANTSFILE))
    {
      std::string filename(config.get_param<std::string>(CONFIG_CONSTANTSFILE));
      LOG4CXX_INFO(logger_, "calib file:" << filename);
      m_loadedConstants = false;
      if(access(filename.c_str(), R_OK)==0)
      {
        rc = m_calibratorSample.loadLatGain(filename);
        if(rc)
        {
            LOG4CXX_ERROR(logger_, "Can not retrieve Lat Gain constants from " << filename);
        }
        else
        {
            rc = m_calibratorSample.loadADCGain(filename);
            rc |= m_calibratorReset.loadADCGain(filename);
            if(rc)
            {
                LOG4CXX_ERROR(logger_, "Can not retrieve ADC Gain constants from " << filename);
            }
            else
            {
                m_loadedConstants = true;
            }
        }
      }
      else
      {
        LOG4CXX_ERROR(logger_, "Can not find / open " << filename);
      }
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
    return PERCIVAL_VERSION_MAJOR;
  }

  int PercivalCalibPlugin::get_version_minor()
  {
    return PERCIVAL_VERSION_MINOR;
  }

  int PercivalCalibPlugin::get_version_patch()
  {
    return PERCIVAL_VERSION_PATCH;
  }

  std::string PercivalCalibPlugin::get_version_short()
  {
    return PERCIVAL_VERSION_STR_SHORT;
  }

  std::string PercivalCalibPlugin::get_version_long()
  {
    return PERCIVAL_VERSION_STR;
  }

  void PercivalCalibPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    if(m_loadedConstants == false)
    {
        LOG4CXX_ERROR(logger_, "calibration constants need to be loaded");
    }
    std::string name = frame->get_meta_data().get_dataset_name();
    LOG4CXX_TRACE(logger_, "got frame " << name);
    if(name == "data")
    {
        boost::shared_ptr<Frame> newfr;
        dimensions_t dims{FRAME_ROWS, FRAME_COLS};
        // temp turn this on until reset frame numbers are fixed on the descrambler.
        if(true || frame->get_meta_data().get_frame_number() == m_resetFrameNumber)
        {
            int sz = dims[0] * dims[1] * sizeof(float);
            newfr.reset(new DataBlockFrame(frame->get_meta_data(), sz));
            newfr->meta_data().set_dataset_name("ecount");
            newfr->meta_data().set_data_type(FrameProcessor::raw_float);

            MemBlockI16 in;
            MemBlockF out;
            in.init(logger_, FRAME_ROWS, FRAME_COLS, frame->get_image_ptr());
            out.init(logger_, FRAME_ROWS, FRAME_COLS, newfr->get_image_ptr());

            LOG4CXX_TRACE(logger_, "Processing calib frame");
            m_calibratorSample.processFrameP(in,out);

            this->push(newfr);
        }
        else
        {
            LOG4CXX_ERROR(logger_, "reset frame does not match sample frame");
        }
    }
    else if(name=="reset")
    {
        MemBlockI16 in;
        in.init(logger_, FRAME_ROWS, FRAME_COLS, frame->get_image_ptr());

        m_calibratorReset.processFrameP(in, m_calibratorSample.m_resetFrame);
        m_resetFrameNumber = frame->meta_data().get_frame_number();
    }
    else
    {
        // other frames pass onto next stage
        this->push(frame);
    }
  }
} /* namespace FrameProcessor */

