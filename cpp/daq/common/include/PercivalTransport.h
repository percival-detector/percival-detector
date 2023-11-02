/*
 * PercivalTransport.h
 *
 *  Created on: Feb 2, 2016
 *      Author: tcn45
 */

#pragma once

namespace PercivalTransport {

    // there are four packets to a row-group which is 7 rows
    static const size_t packet_pixeldata_size  = 4928;
    static const size_t num_primary_packets    = 424;
    static const size_t tail_packet_size       = 0;
    static const size_t num_tail_packets       = 0;
    static const size_t num_subframes          = 2;

    static const size_t num_data_types         = 2;
    static const uint8_t data_type_reset       = 0;

    static const size_t packet_header_size     = 54;
    static const size_t datablock_size_offset  = 0;
    static const size_t packet_type_offset     = 2;
    static const size_t subframe_number_offset = 3;
    static const size_t frame_number_offset    = 4;
    static const size_t packet_number_offset   = 8;
    static const size_t packet_offset_offset   = 10;
    static const size_t frame_info_offset      = 12;

    static const size_t frame_info_size        = 42;

    // this is what appears at the start of a UDP payload from the detector
    // warning: these fields are big-endian on the network, so this struct is unhelpful.
    struct PacketHeaderFields
    {
        // this is the size of what appears after the header.
        uint16_t m_datablock_size;
        uint8_t m_packet_type;
        uint8_t m_subframe_number;
        uint32_t m_frame_number;
        uint16_t m_packet_number;
        uint16_t m_packet_offset;
        uint8_t m_frame_info[frame_info_size];
    }  __attribute__ ((packed));

    static_assert(sizeof(PacketHeaderFields)==packet_header_size, "PacketHeaderFields is malformed");

    // this is what appears at the start of a shared-mem buffer that arrives at the FP.
    // these fields are little-endian on an intel processor.
    typedef struct
    {
        uint32_t frame_number;
        uint32_t frame_state;
        struct timespec frame_start_time;
        uint32_t packets_received;
        uint8_t  frame_info[frame_info_size];
        uint8_t  packet_state[num_data_types][num_subframes][num_primary_packets + num_tail_packets];
    } FrameHeader;

    static const size_t subframe_size       = (num_primary_packets * packet_pixeldata_size)
                                            + (num_tail_packets * tail_packet_size);
    static const size_t data_type_size      = subframe_size * num_subframes;
    static const size_t total_frame_size    = (data_type_size * num_data_types) + sizeof(FrameHeader);
    static const size_t num_frame_packets   = num_subframes * num_data_types *
                                              (num_primary_packets + num_tail_packets);

}

