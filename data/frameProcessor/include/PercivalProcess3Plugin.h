
#ifndef TOOLS_FILEWRITER_PERCIVALPROCESS3PLUGIN_H_
#define TOOLS_FILEWRITER_PERCIVALPROCESS3PLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "PercivalEmulatorDefinitions.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

  /** Processing of Percival Frame objects.
   *
   * The PercivalProcess3Plugin class is responsible for receiving a raw data
   * Frame object and parsing the header information, splitting the raw data into
   * the two "data" and "reset" Frame objects.
   * This one does minimal processing, and is designed for use with the
   * descramber-board which does all the work. It needs to be elaborated to get
   * the improvements in Plugin0.
   */
  class PercivalProcess3Plugin : public FrameProcessorPlugin
  {
  public:
    PercivalProcess3Plugin();
    virtual ~PercivalProcess3Plugin();
    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    void configureProcess(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    bool reset_statistics();
    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

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
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, PercivalProcess3Plugin, "PercivalProcess3Plugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_ */
