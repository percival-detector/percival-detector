/*
 * FrameDecoderUnitTest.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#define BOOST_TEST_MODULE "PercivalFrameDecoderTests"
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <log4cxx/logger.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/simplelayout.h>

#include "PercivalFrameDecoder.h"

class FrameDecoderTestFixture
{
public:
    FrameDecoderTestFixture() :
        logger(log4cxx::Logger::getLogger("FrameDecoderUnitTest"))
    {

    }
    log4cxx::LoggerPtr logger;
};
BOOST_FIXTURE_TEST_SUITE(FrameDecoderUnitTest, FrameDecoderTestFixture);

BOOST_AUTO_TEST_CASE( PercivalDecoderTest )
{
    boost::shared_ptr<FrameReceiver::FrameDecoderUDP> decoder(new FrameReceiver::PercivalFrameDecoder());
    OdinData::IpcMessage config_msg;
    decoder->init(logger, config_msg);

    BOOST_TEST_MESSAGE("Percival buffer size is specified as " << decoder->get_frame_buffer_size());
    BOOST_TEST_MESSAGE("Percival frame header size is specified as " << decoder->get_frame_header_size());
    BOOST_TEST_MESSAGE("Percival packet header size is specified as " << decoder->get_packet_header_size());

    FrameReceiver::PercivalFrameDecoder* percivalDecoder = reinterpret_cast<FrameReceiver::PercivalFrameDecoder*>(decoder.get());

    void* packet_header = decoder->get_packet_header_buffer();
    BOOST_CHECK_NE(packet_header, reinterpret_cast<void*>(0));

    // Hand craft packet header to check accessor methods cope with field alignment
    uint8_t* hdr_raw = reinterpret_cast<uint8_t*>(packet_header);

    uint16_t datablock_size = 4874;
    uint8_t  packet_type = 1;
    uint8_t  subframe_number=15;
    uint32_t frame_number = 0x12345678;
    uint16_t packet_number = 0xaa55;

    hdr_raw[0] = static_cast<uint8_t>((datablock_size >> 0) & 0xFF);
    hdr_raw[1] = static_cast<uint8_t>((datablock_size >> 8) & 0xFF);
    hdr_raw[2] = packet_type;
    hdr_raw[3] = subframe_number;
    hdr_raw[4] = static_cast<uint8_t>((frame_number >> 24) & 0xFF);
    hdr_raw[5] = static_cast<uint8_t>((frame_number >> 16) & 0xFF);
    hdr_raw[6] = static_cast<uint8_t>((frame_number >>  8) & 0xFF);
    hdr_raw[7] = static_cast<uint8_t>((frame_number >>  0) & 0xFF);
    hdr_raw[8] = static_cast<uint8_t>((packet_number >> 8) & 0xFF);
    hdr_raw[9] = static_cast<uint8_t>((packet_number >> 0) & 0xFF);

    BOOST_CHECK_EQUAL(percivalDecoder->get_datablock_size(), datablock_size);
    BOOST_CHECK_EQUAL(percivalDecoder->get_packet_type(), packet_type);
    BOOST_CHECK_EQUAL(percivalDecoder->get_subframe_number(), subframe_number);
    BOOST_CHECK_EQUAL(percivalDecoder->get_packet_number(), packet_number);
    BOOST_CHECK_EQUAL(percivalDecoder->get_frame_number(), frame_number);

}

BOOST_AUTO_TEST_SUITE_END();

