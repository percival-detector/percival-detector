/*
 * PercivalFrameDecoder.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#include "PercivalFrameDecoder.h"
#include "gettime.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h>
#include "percival_version.h"

using namespace FrameReceiver;

static const int DUMMY_BUFFER = -10;
static const int NOFRAME = -1;

PercivalFrameDecoder::PercivalFrameDecoder() :
        FrameDecoderUDP(),
		current_frame_num_(NOFRAME),
		current_frame_buffer_id_(-1),
		current_frame_buffer_(0),
		current_frame_header_(0),
    bad_packets_seen_(0)
{
    current_packet_header_.reset(new uint8_t[PercivalTransport::packet_header_size]);
    dropped_frame_buffer_.reset(new uint8_t[PercivalTransport::total_frame_size]);
}

PercivalFrameDecoder::~PercivalFrameDecoder()
{
}

int PercivalFrameDecoder::get_version_major()
{
  return PERCIVAL_VERSION_MAJOR;
}

int PercivalFrameDecoder::get_version_minor()
{
  return PERCIVAL_VERSION_MINOR;
}

int PercivalFrameDecoder::get_version_patch()
{
  return PERCIVAL_VERSION_PATCH;
}

std::string PercivalFrameDecoder::get_version_short()
{
  return PERCIVAL_VERSION_STR_SHORT;
}

std::string PercivalFrameDecoder::get_version_long()
{
  return PERCIVAL_VERSION_STR;
}

//(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
//{
//  FrameDecoderUDP::init(logger,config_msg);
//
void PercivalFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
	FrameDecoder::init(logger, config_msg);

    if (enable_packet_logging_) {
        LOG4CXX_INFO(packet_logger_, "PktHdr: SourceAddress");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               SourcePort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     DestinationPort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      DataSize [2 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     PacketType [1 Byte]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  SubframeNumber [1 Byte]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  |  FrameNumber [4 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  |  |             PacketNumber [2 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  |  |             |     PacketOffset [2 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  |  |             |     |     Info [42 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |     |  |  |             |     |     |");
    }
}

const size_t PercivalFrameDecoder::get_frame_buffer_size(void) const
{
    return PercivalTransport::total_frame_size;
}

const size_t PercivalFrameDecoder::get_frame_header_size(void) const
{
    return sizeof(PercivalTransport::FrameHeader);
}

const size_t PercivalFrameDecoder::get_packet_header_size(void) const
{
    return PercivalTransport::packet_header_size;
}

void* PercivalFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header_.get();
}

void PercivalFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    //TODO validate header size and content, handle incoming new packet buffer allocation etc

    // Dump raw header if packet logging enabled
    if (enable_packet_logging_)
    {
        std::stringstream ss;
        uint8_t* hdr_ptr = raw_packet_header();
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex;
        for (unsigned int hdr_byte = 0; hdr_byte < PercivalTransport::packet_header_size; hdr_byte++)
        {
            if (hdr_byte % 8 == 0) {
                ss << "  ";
            }
            ss << std::setw(2) << std::setfill('0') << (unsigned int)*hdr_ptr << " ";
            hdr_ptr++;
        }
        ss << std::dec;
        LOG4CXX_INFO(packet_logger_, ss.str());
    }

	  int frame = static_cast<int>(get_frame_number());
	  uint16_t packet_number = get_packet_number();
	  uint8_t  subframe = get_subframe_number();
	  uint8_t  type = get_packet_type();
    uint16_t datablock_size = get_datablock_size();

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Got packet header:"
            << " type: "     << (int)type << " subframe: " << (int)subframe
            << " packet: "   << packet_number    << " frame: "    << frame
    );

    if (frame != current_frame_num_)
    {
      current_frame_num_ = frame;
      bool bNeedInitializeHeader = false;

    	if (frame_buffer_map_.count(current_frame_num_) == 0 && frames_we_drop_.count(current_frame_num_) == 0)
    	{
        // new frame appears, allocate a buffer for it.
        if (empty_buffer_queue_.empty())
        {
            LOG4CXX_ERROR(logger_, "First packet from frame " << current_frame_num_ << " but no free buffers. Dropping frame.");
            frames_we_drop_[current_frame_num_] = DUMMY_BUFFER;
            frames_dropped_ += 1;
        }
	      else
	      {
            current_frame_buffer_id_ = empty_buffer_queue_.front();
            empty_buffer_queue_.pop();
            frame_buffer_map_[current_frame_num_] = current_frame_buffer_id_;

            LOG4CXX_DEBUG_LEVEL(2, logger_, "First packet from frame " << current_frame_num_ << " detected, allocating frame buffer ID " << current_frame_buffer_id_);
	      }
        bNeedInitializeHeader = true;
    	}

        // select the right buffer for this frame
    	if(frame_buffer_map_.count(current_frame_num_))
    	{
    		current_frame_buffer_id_ = frame_buffer_map_[current_frame_num_];
        current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
    	}
      else if(frames_we_drop_.count(current_frame_num_))
      {
          current_frame_buffer_id_ = DUMMY_BUFFER;
          current_frame_buffer_ = dropped_frame_buffer_.get();
      }
      current_frame_header_ = reinterpret_cast<PercivalTransport::FrameHeader*>(current_frame_buffer_);

      // initialize the header if it's a new one
      if(bNeedInitializeHeader)
      {
 	        // Initialise frame header
          current_frame_header_->frame_number = current_frame_num_;
          current_frame_header_->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
          current_frame_header_->packets_received = 0;
          memset(current_frame_header_->packet_state, 0, PercivalTransport::num_frame_packets);
          memcpy(current_frame_header_->frame_info, get_frame_info(), PercivalTransport::frame_info_size);
          gettime(reinterpret_cast<struct timespec*>(&(current_frame_header_->frame_start_time)));
      }
    }
}

inline uint32_t PercivalFrameDecoder::get_packet_offset_in_frame(uint8_t type, uint8_t subframe, uint16_t packet) const
{
    return get_frame_header_size() +
    (PercivalTransport::data_type_size * type) +
    (PercivalTransport::subframe_size * subframe) +
    (PercivalTransport::packet_pixeldata_size * packet);
}

void* PercivalFrameDecoder::get_next_payload_buffer(void) const
{
    uint8_t* next_receive_location;

    next_receive_location =
        reinterpret_cast<uint8_t*>(current_frame_buffer_) +
        get_packet_offset_in_frame(get_packet_type(), get_subframe_number(), get_packet_number());

#if DOIT2
// this is a different arrangement of the data in the frame, and saves us
// needing to do so much in the FP.
        next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_) +
            get_frame_header_size() +
            (PercivalTransport::data_type_size * get_packet_type()) +
            PercivalTransport::packet_pixeldata_size * 
            (2*get_packet_number() + get_subframe_number());
#endif
    
    return reinterpret_cast<void*>(next_receive_location);
}

size_t PercivalFrameDecoder::get_next_payload_size(void) const
{
    // the recvmsg function stops at the end of a udp packet, so this may be seen as a maximum buffer size.
    return PercivalTransport::packet_pixeldata_size;
}

FrameDecoder::FrameReceiveState PercivalFrameDecoder::process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;
    if(current_packet_valid(bytes_received) == false)
    {
      ++bad_packets_seen_;
    }
    // we must check the current frame buffer is valid or we could release the dummy buffer to the FP!
    else if(current_frame_buffer_id_ != DUMMY_BUFFER)
    {
      current_frame_header_->packets_received++;

	    if (current_frame_header_->packets_received == PercivalTransport::num_frame_packets)
	    {
	        // Set frame state accordingly
		    frame_state = FrameDecoder::FrameReceiveStateComplete;

		    // Complete frame header
		    current_frame_header_->frame_state = frame_state;

		    // Erase frame from buffer map
		    frame_buffer_map_.erase(current_frame_num_);

        // Notify main thread that frame is ready
        ready_callback_(current_frame_buffer_id_, current_frame_num_);
        LOG4CXX_INFO(logger_, "Frame " << current_frame_num_ << " in buffer " << current_frame_buffer_id_ << " released ok");

		    // Reset current frame seen ID so that if next frame has same number (e.g. repeated
		    // sends of single frame 0), it is detected properly
		    current_frame_num_ = NOFRAME;
	    }
    }

	return frame_state;
}

void PercivalFrameDecoder::monitor_buffers(void)
{
    int frames_timedout = 0;
    struct timespec current_time;

    gettime(&current_time);

    // Loop over frame buffers currently in map and forward old ones
    std::map<int, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
    while (buffer_map_iter != frame_buffer_map_.end())
    {
        int frame_num = buffer_map_iter->first;
        int      buffer_id = buffer_map_iter->second;
        void*    buffer_addr = buffer_manager_->get_buffer_address(buffer_id);
        PercivalTransport::FrameHeader* frame_header = reinterpret_cast<PercivalTransport::FrameHeader*>(buffer_addr);
        unsigned int elapse_ms = this->elapsed_ms(frame_header->frame_start_time, current_time);
        if (elapse_ms > frame_timeout_ms_)
        {
            LOG4CXX_WARN(logger_, "Frame " << frame_num << " in buffer " << buffer_id
                    << " timed out after " << elapse_ms << "ms"
                    << " with " << frame_header->packets_received << " packets received");

            frame_header->frame_state = FrameReceiveStateTimedout;
            // we blank areas of the frame to make it clear they contain invalid data
            for(int type=0;type<PercivalTransport::num_data_types;++type)
            {
              for(int subframe=0;subframe<PercivalTransport::num_subframes;++subframe)
              {
                int missing_packet_count = 0;
                for(int packetid=0;packetid<PercivalTransport::num_primary_packets;++packetid)
                {
                  // check for specific packets missing
                  if(current_frame_header_->packet_state[type][subframe][packetid]==0)
                  {
                    // blank the memory with 0xff which can not be created by the detector
                    uint8_t* packet_location = reinterpret_cast<uint8_t*>(buffer_addr) +
                      get_packet_offset_in_frame(type, subframe, packetid);
                    memset(packet_location, 0xff, PercivalTransport::packet_pixeldata_size);
                    // we could log individual packets here, but it would be slow
                    ++missing_packet_count;
                  }
                }
                LOG4CXX_DEBUG(logger_, "type " << type << " subframe " << subframe << " num packets missing " << missing_packet_count);
              }
            }

            ready_callback_(buffer_id, frame_num);
            frames_timedout++;

            frame_buffer_map_.erase(buffer_map_iter++);
        }
        else
        {
            buffer_map_iter++;
        }
    }
    if (frames_timedout)
    {
        LOG4CXX_WARN(logger_, "Released " << frames_timedout << " timed out incomplete frames");
        // we want to zap the cached frame-seen in case it has now gone.
		    current_frame_num_ = NOFRAME;
    }
    frames_timedout_ += frames_timedout;

    LOG4CXX_DEBUG_LEVEL(3, logger_, get_num_mapped_buffers() << " frame buffers in use, "
            << get_num_empty_buffers() << " empty buffers available, "
            << frames_timedout_ << " incomplete frames timed out");

    // iterate over frames that go to dummy buffer and delete the old ones
    while(3<frames_we_drop_.size())
    {
        frames_we_drop_.erase(frames_we_drop_.begin());
    }
}

void PercivalFrameDecoder::get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg)
{
  status_msg.set_param(param_prefix + "bad_packets", this->bad_packets_seen_);
}

uint16_t PercivalFrameDecoder::get_datablock_size(void) const
{
    uint16_t datablock_size = *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalTransport::datablock_size_offset));
    return ntohs(datablock_size);  
}

uint8_t PercivalFrameDecoder::get_packet_type(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalTransport::packet_type_offset));
}

uint8_t PercivalFrameDecoder::get_subframe_number(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalTransport::subframe_number_offset));
}

uint32_t PercivalFrameDecoder::get_frame_number(void) const
{
    uint32_t frame_number_raw = *(reinterpret_cast<uint32_t*>(raw_packet_header()+PercivalTransport::frame_number_offset));
    return ntohl(frame_number_raw);
}

uint16_t PercivalFrameDecoder::get_packet_number(void) const
{
	uint16_t packet_number_raw = *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalTransport::packet_number_offset));
    return ntohs(packet_number_raw);
}

uint16_t PercivalFrameDecoder::get_packet_offset(void) const
{
    return *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalTransport::packet_offset_offset));
}

inline uint8_t* PercivalFrameDecoder::get_frame_info(void) const
{
    return (reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalTransport::frame_info_offset));
}

inline uint8_t* PercivalFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_packet_header_.get());
}


inline unsigned int PercivalFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}

void PercivalFrameDecoder::reset_statistics(void)
{
    frames_we_drop_.clear();
    bad_packets_seen_ = 0;
    
    FrameDecoderUDP::reset_statistics();
}

inline bool PercivalFrameDecoder::current_packet_valid(size_t bytes_received)
{
  bool valid = true;
  uint32_t frame_num = get_frame_number();
  uint16_t packet_number = get_packet_number();
  // these are uint8_ts but for printing we make them ints
  int  subframe = get_subframe_number();
  int  type = get_packet_type();
  uint16_t datablock_size = get_datablock_size();

  // check the header-info for bad parameters

  if(__builtin_expect(bytes_received != PercivalTransport::packet_pixeldata_size + PercivalTransport::packet_header_size, false))
  {
      LOG4CXX_ERROR(logger_, "bad packet has actual size:" << bytes_received << " and claims to have db-size " << get_datablock_size());
      valid = false;
  }

  if(__builtin_expect(datablock_size != PercivalTransport::packet_pixeldata_size, false))
  {
      LOG4CXX_ERROR(logger_, "Packet num " << packet_number << " claims to have size " << get_datablock_size());
      valid = false;
  }

  if(__builtin_expect(type >= PercivalTransport::num_data_types, false))
  {
      LOG4CXX_ERROR(logger_, "Packet num " << packet_number << " claims to have type " << type);
      valid = false;
  }

  if(__builtin_expect(subframe >= PercivalTransport::num_subframes, false))
  {
      // reference frames have subframe 128, but they are not used any more.
      LOG4CXX_ERROR(logger_, "Packet num " << packet_number << " claims to have subframe " << subframe);
      valid = false;
  }

  if(frame_num)
  {
    // this may be done later
    // we could keep a map of frame_num to num_frames_released to see if we get duplicate packets for a frame.
  }

  if (__builtin_expect(packet_number >= PercivalTransport::num_primary_packets, false))
  {
      LOG4CXX_ERROR(logger_, "Packet claims to have packet_number " << packet_number);
      valid = false;
  }

  if(valid && __builtin_expect(current_frame_header_->packet_state[type][subframe][packet_number], false))
  {
      LOG4CXX_ERROR(logger_, "Packet type " << type << " subframe " << subframe << " packet " << packet_number << " has already been seen");
      valid = false;
  }

  if(__builtin_expect(valid, true))
  {
      current_frame_header_->packet_state[type][subframe][packet_number] = 1;
  }
  else
  {
    // this is really just a separator to distinguish the packets in the logfile.
      LOG4CXX_WARN(logger_, "BAD PACKET REJECTED");
  }



  return valid;
}

