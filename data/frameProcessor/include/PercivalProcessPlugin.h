/*
 * PercivalProcessPlugin.h
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_
#define TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "PercivalTransport.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

  /** Processing of Percival Frame objects.
   *
   * The PercivalProcessPlugin class is currently responsible for receiving a raw data
   * Frame object and parsing the header information, before splitting the raw data into
   * the two "data" and "reset" Frame objects.
   * This one is legacy code and the rearranging it does seems to be pointless.
   */
  class PercivalProcessPlugin : public FrameProcessorPlugin
  {
  public:
    PercivalProcessPlugin();
    virtual ~PercivalProcessPlugin();
    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    void configureProcess(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);
    bool reset_statistics();
    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

    void process_frame(boost::shared_ptr<Frame> frame);

  private:
    /** Configuration constant for process related items */
    static const std::string CONFIG_PROCESS;
    /** Configuration constant for number of processes */
    static const std::string CONFIG_PROCESS_NUMBER;
    /** Configuration constant for this process rank */
    static const std::string CONFIG_PROCESS_RANK;

    void processInfoField(const PercivalTransport::FrameHeader* hdrPtr, FrameMetaData md);
    void addFrameNumField(const PercivalTransport::FrameHeader* hdrPtr, FrameMetaData md);


    /** Pointer to logger */
    LoggerPtr logger_;

    size_t concurrent_processes_;
    size_t concurrent_rank_;

    // this is ~the frame-number of the first frame after a reset
    // needs to be int64 to hold 32 bits of framenum and 1 bit for negative.
    // it is int64 in FrameMetaData class
    int64_t frame_base_;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, PercivalProcessPlugin, "PercivalProcessPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_ */
