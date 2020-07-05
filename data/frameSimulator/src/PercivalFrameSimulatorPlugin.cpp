

#include "version.h"
#include "PercivalFrameSimulatorPlugin.h"
#include "PercivalEmulatorDefinitions.h"
#include "FrameSimulatorOption.h"

#include "DebugLevelLogger.h"

#include <boost/lexical_cast.hpp>

#include <arpa/inet.h>

#include <string>
#include <cstdlib>
#include <time.h>
#include <iostream>
#include <algorithm>


namespace FrameSimulator {

    static const FrameSimulatorOption<int> opt_packetfillmode("fill-mode", "How to fill created packets:\n 0 - packet n repeats <n>\n 1 - each pixel increments the previous; first pixel of reset frame is 0");

    /** Construct an PercivalFrameSimulatorPlugin
    * setup an instance of the logger
    * initialises data and frame counts
    */
    PercivalFrameSimulatorPlugin::PercivalFrameSimulatorPlugin() : FrameSimulatorPluginUDP() {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.PercivalFrameSimulatorPlugin");
        logger_->setLevel(Level::getAll());

        m_numPacketsFromPcap = 0;
        m_numFramesToCreate = 0;


    }

    void PercivalFrameSimulatorPlugin::populate_options(po::options_description& config)
    {
        FrameSimulatorPluginUDP::populate_options(config);
        LOG4CXX_DEBUG(logger_, "populating options");
        opt_packetfillmode.add_option_to(config);
    }

    bool PercivalFrameSimulatorPlugin::setup(const po::variables_map& vm)
    {
        LOG4CXX_DEBUG(logger_, "setup options");
        m_packetFillMode = ePacketId;
        if(opt_packetfillmode.is_specified(vm))
        {
            m_packetFillMode = opt_packetfillmode.get_val(vm);
            LOG4CXX_INFO(logger_, "packet fill mode specified " << m_packetFillMode);
            if(m_packetFillMode >= eMax)
            {
                LOG4CXX_ERROR(logger_, "packet fill mode invalid:" << m_packetFillMode);
                return false;
            }
        }
        else
        {
            LOG4CXX_DEBUG(logger_, "packet fill mode unspecified, defaulting to 0");
          //  return false;
        }
        // this func calls create/extract_frames so must be last
        return FrameSimulatorPluginUDP::setup(vm);
        // we can call LOG4CXX_DEBUG_LEVEL after this
    }

    /** This is called by the framework with the payload of each packet in the pcap
     * \param[in] data - 1x packet payload; temp buffer
     * \param[in] size - but we know what we expect the size to be
     */
    void PercivalFrameSimulatorPlugin::extract_frames(const u_char *indata, const int &size)
    {
        LOG4CXX_DEBUG_LEVEL(4, logger_, "Copying packet #" << m_numPacketsFromPcap);
        const int idealSize = PercivalEmulator::primary_packet_size + PercivalEmulator::packet_header_size;
        LOG4CXX_ASSERT(logger_, (idealSize == size), "packet " << m_numPacketsFromPcap << " in pcap file is the wrong size");

        if(PercivalEmulator::packet_header_size <= size)
        {
            void* buf = malloc(size);
            memcpy(buf, indata, size);

            boost::shared_ptr<Packet> pkt(new Packet());
            pkt->data = static_cast<u_char*>(buf);
            pkt->size = size;

            // todo this will come from reading the pheader with ntohl
            PercivalEmulator::PacketHeaderFields const* pHeader = 
                                reinterpret_cast<PercivalEmulator::PacketHeaderFields const*>(indata);
            int frameNum = pHeader->m_frame_number;
            bool bFound = false;
            for(int i=0;i<frames_.size() && bFound==false;++i)
            {
                if(frameNum == frames_[i].frame_number)
                {
                    LOG4CXX_DEBUG_LEVEL(5, logger_, "Matched to frame id " << frameNum);
                    frames_[i].packets.push_back(pkt);
                    bFound = true;  
                }
            }

            if(bFound == false)
            {
                LOG4CXX_DEBUG_LEVEL(4, logger_, "New frame id " << frameNum << ". #frames: " << frames_.size()+1);
                frames_.push_back( UDPFrame(frameNum) );
                frames_.back().packets.push_back(pkt);
            }
            
            ++m_numPacketsFromPcap;
        }
    }

    /** Creates a number of frames
     *
     * @param num_frames - number of frames to create
     */
    void PercivalFrameSimulatorPlugin::create_frames(const int &num_frames)
    {
        m_numFramesToCreate = num_frames;
        LOG4CXX_DEBUG(logger_, "Creating frame(s) " << m_numFramesToCreate);
        const int size = PercivalEmulator::primary_packet_size + PercivalEmulator::packet_header_size;

        for(int fr=0;fr<m_numFramesToCreate;++fr)
        {
            frames_.push_back( UDPFrame(fr) );
            uint16_t pixelCount;
            
            for(int pt=0;pt<PercivalEmulator::num_data_types;++pt)
            {
                // reset the count so the start of each data / reset frame is zero-pixel
                pixelCount = 0;
                for(int sf=0;sf<PercivalEmulator::num_subframes;++sf)
                {
                    for(int pk=0;pk<PercivalEmulator::num_primary_packets;++pk)
                    {
                        boost::shared_ptr<Packet> pkt(new Packet());
                        void* buf = malloc(size);
                        memset(buf,0x99,size);
                        pkt->data = static_cast<u_char*>(buf);
                        pkt->size = size;

                        PercivalEmulator::PacketHeaderFields* pHeader = 
                            static_cast<PercivalEmulator::PacketHeaderFields*>(buf);

                        pHeader->m_packet_type = pt;
                        pHeader->m_subframe_number = sf;
                        pHeader->m_frame_number = htonl(fr);
                        pHeader->m_packet_number = htons(pk);
                        memset(pHeader->m_frame_info, 0x98, PercivalEmulator::frame_info_size);

                        uint16_t* pixels = static_cast<uint16_t*>(buf)+PercivalEmulator::packet_header_size/2;
                        for(int pix=0;pix<PercivalEmulator::primary_packet_size/2;++pix)
                        {
                            if(m_packetFillMode == ePacketId)
                            {
                                pixels[pix] = pk + sf * PercivalEmulator::num_primary_packets;
                            }
                            else if(m_packetFillMode == eIncrementing)
                            {
                                pixels[pix] = pixelCount;
                            }
                            ++pixelCount;
                        }
                        LOG4CXX_DEBUG_LEVEL(5, logger_, "Creating packet " << pk << " for subframe " << sf);
                        frames_.back().packets.push_back(pkt);
                    }
                }
            }
        }
    }

   /**
    * Get the plugin major version number.
    *
    * \return major version number as an integer
    */
    int PercivalFrameSimulatorPlugin::get_version_major() {
        return ODIN_DATA_VERSION_MAJOR;
    }

    /**
     * Get the plugin minor version number.
     *
     * \return minor version number as an integer
     */
    int PercivalFrameSimulatorPlugin::get_version_minor() {
        return ODIN_DATA_VERSION_MINOR;
    }

    /**
     * Get the plugin patch version number.
     *
     * \return patch version number as an integer
     */
    int PercivalFrameSimulatorPlugin::get_version_patch() {
        return ODIN_DATA_VERSION_PATCH;
    }

    /**
     * Get the plugin short version (e.g. x.y.z) string.
     *
     * \return short version as a string
     */
    std::string PercivalFrameSimulatorPlugin::get_version_short() {
        return ODIN_DATA_VERSION_STR_SHORT;
    }

    /**
     * Get the plugin long version (e.g. x.y.z-qualifier) string.
     *
     * \return long version as a string
     */
    std::string PercivalFrameSimulatorPlugin::get_version_long() {
        return ODIN_DATA_VERSION_STR;
    }

}
