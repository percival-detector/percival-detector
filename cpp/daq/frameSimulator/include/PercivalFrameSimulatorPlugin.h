#ifndef FRAMESIMULATOR_PERCIVALFRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_PERCIVALFRAMESIMULATORPLUGIN_H

#include "FrameSimulatorPluginUDP.h"

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>
// todo move this elsewhere
#include "ClassLoader.h"
#include <map>
#include <string>
#include <stdexcept>


namespace FrameSimulator {

    /** PercivalFrameSimulatorPlugin
     *  this class relies heavily on the framework in the superclass. It only provides
     *   two methods:
     *  extract_frames() is called per packet in super::setup() if a pcap file is specified
     *  create_frames() is called in super::setup otherwise
     */
    class PercivalFrameSimulatorPlugin : public FrameSimulatorPluginUDP {

    public:

        PercivalFrameSimulatorPlugin();

        virtual void populate_options(po::options_description& config);
        virtual bool setup(const po::variables_map& vm);

        virtual int get_version_major();
        virtual int get_version_minor();
        virtual int get_version_patch();
        virtual std::string get_version_short();
        virtual std::string get_version_long();

    protected:
        virtual void extract_frames(const u_char* data, const int& size);
        virtual void create_frames(const int &num_frames);

    private:
        enum PacketFillMode
        {
            ePacketId = 0,
            eIncrementing,
            eMax,
        };
        /** Pointer to logger **/
        LoggerPtr logger_;

        int m_numPacketsFromPcap;
        int m_numFramesToCreate;
        int m_packetFillMode;

    };

    REGISTER(FrameSimulatorPlugin, PercivalFrameSimulatorPlugin, "PercivalFrameSimulatorPlugin");

}

#endif
