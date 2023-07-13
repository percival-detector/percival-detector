/*
 * PercivalFrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#pragma once

#include "FrameDecoderUDP.h"
#include "PercivalTransport.h"
#include <iostream>
#include <stdint.h>
#include <time.h>


namespace FrameReceiver
{
    class PercivalFrameDecoder : public FrameDecoderUDP
    {
    public:

        PercivalFrameDecoder();
        ~PercivalFrameDecoder();

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
        void reset_statistics(void);

        void* get_packet_header_buffer(void);

        uint16_t get_datablock_size(void) const;
        uint8_t get_packet_type(void) const;
        uint8_t get_subframe_number(void) const;
        uint32_t get_frame_number(void) const;
        uint16_t get_packet_number(void) const;
        uint16_t get_packet_offset(void) const;
        uint8_t* get_frame_info(void) const;

    private:
        uint32_t get_packet_offset_in_frame(uint8_t type, uint8_t subframe, uint16_t packet) const;
        uint8_t* raw_packet_header(void) const;
        unsigned int elapsed_ms(struct timespec& start, struct timespec& end);

        boost::shared_ptr<void> current_packet_header_;
        boost::shared_ptr<void> dropped_frame_buffer_;

        int bad_packets_seen_;
        int current_frame_num_;
        int current_frame_buffer_id_;
        void* current_frame_buffer_;
        //! this function checks all the packet parameters like datablock size are within the bounds we expect.
        //! It was added when we saw strange packets with the wrong packet numbers appearing.
        //! It has the side-effect of recording the packet arrival in the frame buffer.
        bool current_packet_valid(size_t bytes_received);
        PercivalTransport::FrameHeader* current_frame_header_;

        std::map<int,int> frames_we_drop_;

    };

} // namespace FrameReceiver

