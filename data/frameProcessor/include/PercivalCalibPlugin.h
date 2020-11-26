
#ifndef TOOLS_FILEWRITER_PercivalCalibPlugin_H_
#define TOOLS_FILEWRITER_PercivalCalibPlugin_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "CalibratorReset.h"
#include "CalibratorSample.h"
#include "FrameProcessorPlugin.h"
#include "PercivalTransport.h"
#include "ClassLoader.h"

#include <thread>

namespace FrameProcessor
{

  /** Test class to generate test frames
   *
   */
  class PercivalCalibPlugin : public FrameProcessorPlugin
  {
  public:
    PercivalCalibPlugin();
    virtual ~PercivalCalibPlugin();
    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    bool reset_statistics();
    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

  private:
    void process_frame(boost::shared_ptr<Frame> frame);

    /** Pointer to logger */
    LoggerPtr logger_;

    size_t concurrent_processes_;
    size_t concurrent_rank_;
    CalibratorSample m_calibratorSample;
    CalibratorReset m_calibratorReset;

    /* Frame counter */
    uint32_t frame_counter_;
    // the frame-number of the last reset frame
    uint32_t m_resetFrameNumber;
    bool m_loadedConstants;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, PercivalCalibPlugin, "PercivalCalibPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_PercivalCalibPlugin_H_ */
