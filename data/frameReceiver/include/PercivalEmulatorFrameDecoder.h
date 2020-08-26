/*
 * PercivalEmulatorFrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#ifndef INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_
#define INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_

#include "FrameDecoderUDP.h"
#include "PercivalEmulatorDefinitions.h"
#include <iostream>
#include <stdint.h>
#include <time.h>


namespace FrameReceiver
{
    class PercivalEmulatorFrameDecoder : public FrameDecoderUDP
    {
    public:

        PercivalEmulatorFrameDecoder();
        ~PercivalEmulatorFrameDecoder();

        int get_version_major();
        int get_version_minor();
        int get_version_patch();
        std::string get_version_short();
        std::string get_version_long();

        void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);

        const size_t get_frame_buffer_size(void) const;
        const size_t get_frame_header_size(void) const;

        inline const bool requires_header_peek(void) const { return true; };
        const size_t get_packet_header_size(void) const;
        void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr);

        void* get_next_payload_buffer(void) const;
        size_t get_next_payload_size(void) const;
        FrameDecoder::FrameReceiveState process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr);

        void monitor_buffers(void);
        void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);
        // TODO: void reset_statistics(void);

        void* get_packet_header_buffer(void);

#ifdef P2M_EMULATOR_NEW_FIRMWARE
        uint16_t get_pixel_data_size(void) const;
#endif
        uint8_t get_packet_type(void) const;
        uint8_t get_subframe_number(void) const;
        uint32_t get_frame_number(void) const;
        uint16_t get_packet_number(void) const;
#ifdef P2M_EMULATOR_NEW_FIRMWARE
        uint16_t get_packet_offset(void) const;
#endif
        uint8_t* get_frame_info(void) const;

    private:

        uint8_t* raw_packet_header(void) const;
        unsigned int elapsed_ms(struct timespec& start, struct timespec& end);

        boost::shared_ptr<void> current_packet_header_;
        boost::shared_ptr<void> dropped_frame_buffer_;
        boost::shared_ptr<void> reference_packet_buffer_;

        int current_frame_seen_;
        int current_frame_buffer_id_;
        void* current_frame_buffer_;
        PercivalEmulator::FrameHeader* current_frame_header_;

        unsigned int reference_packets_seen_;

        std::map<int,int> frames_we_drop_;

    };

} // namespace FrameReceiver

#endif /* INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_ */
