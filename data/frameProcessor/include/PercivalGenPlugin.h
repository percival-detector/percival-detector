
#ifndef TOOLS_FILEWRITER_PercivalGenPlugin_H_
#define TOOLS_FILEWRITER_PercivalGenPlugin_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "PercivalEmulatorDefinitions.h"
#include "ClassLoader.h"

#include "FrameMem.h"

#include <thread>

namespace FrameProcessor
{

  /** Test class to generate test frames
   *
   */
  class PercivalGenPlugin : public FrameProcessorPlugin
  {
  public:
    PercivalGenPlugin();
    virtual ~PercivalGenPlugin();
    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    void configureProcess(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    bool reset_statistics();
    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

    void threadfn();
  private:
    /** Configuration constant for process related items */
    static const std::string CONFIG_PROCESS;
    /** Configuration constant for number of processes */
    static const std::string CONFIG_PROCESS_NUMBER;
    /** Configuration constant for this process rank */
    static const std::string CONFIG_PROCESS_RANK;

    void process_frame(boost::shared_ptr<Frame> frame);

    /** Pointer to logger */
    LoggerPtr logger_;

    size_t concurrent_processes_;
    size_t concurrent_rank_;

    /* Frame counter */
    uint32_t frame_counter_;

    std::thread mythread_;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, PercivalGenPlugin, "PercivalGenPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_PercivalGenPlugin_H_ */
