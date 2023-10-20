//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef MPEG_TS_MEDIA_SRC_MPEG_TS_H
#define MPEG_TS_MEDIA_SRC_MPEG_TS_H

#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

#define TS_PACKET_SIZE   188 // .ts
#define M2TS_PACKET_SIZE 192 // .m2ts
#define TS_SYNC_BYTE     0x47

using data_t = std::string;

// clang-format off
enum TS_PID : uint16_t {
    PID_PAT  = 0x0000, /// Program Association Table
    PID_CAT  = 0x0001, /// Conditional Access Table
    PID_TSDT = 0x0002, /// Transport Stream Description Table
    PID_IPMP = 0x0003, /// IPMP control information table
    PID_ASI  = 0x0004, /// Adaptive Streaming Information
    PID_NIT  = 0x0010, /// Network Information Table | ST
    PID_SDT  = 0x0011, /// Service Description Table | BAT | ST
};

enum TS_TID : uint8_t {
    TID_PAS   = 0x00, // Program Association section
    TID_CAS   = 0x01, // Conditional Access section
    TID_PMS   = 0x02, // Program Map section
    TID_TSDS  = 0x03, // Transport Stream Description section
    TID_NIS   = 0x40, // Network Information section - actual network
    TID_NIS_O = 0x41, // Network Information section - other network
    TID_SDS   = 0x42, // Service Description section - actual TS
    TID_SDS_O = 0x46, // Service Descrition section - other TS
};

enum StreamType : uint8_t {
    STREAM_TYPE_RESERVED        = 0x00, // ITU-T | ISO/IEC Reserved
    STREAM_TYPE_VIDEO_MPEG1     = 0x01, // ISO/IEC 11172-2 Video | MPEG-1 video | mpeg1video
    STREAM_TYPE_VIDEO_MPEG2     = 0x02, // Rec. ITU-T H.262 | ISO/IEC 13818-2 Video | MPEG-2 video | mpeg2video
    STREAM_TYPE_AUDIO_MPEG1     = 0x03, // ISO/IEC 11172-3 Audio | mp3 or mp2
    STREAM_TYPE_AUDIO_MPEG2     = 0x04, // ISO/IEC 13818-3 Audio
    STREAM_TYPE_PRIVATE_SECTION = 0x05, // Rec. ITU-T H.222.0 | ISO/IEC 13818-1 private_sections
    STREAM_TYPE_PRIVATE_DATA    = 0x06, // Rec. ITU-T H.222.0 | ISO/IEC 13818-1 PES packets containing private data
    STREAM_TYPE_AUDIO_AAC       = 0x0f, // ISO/IEC 13818-7 AAC Audio with ADTS transport syntax
    STREAM_TYPE_VIDEO_MPEG4     = 0x10, // ISO/IEC 14496-2 Visual | MPEG-4 part 2 | mpeg4
    STREAM_TYPE_AUDIO_AAC_LATM  = 0x11, // ISO/IEC 14496-3 Audio with the LATM transport syntax
    STREAM_TYPE_METADATA        = 0x15, // Metadata carried in PES packets
    STREAM_TYPE_VIDEO_H264      = 0x1b, // Rec. ITU-T H.264 | ISO/IEC 14496-10 AVC
    STREAM_TYPE_VIDEO_HEVC      = 0x24, // Rec. ITU-T H.265 | ISO/IEC 23008-2 HEVC video stream
    STREAM_TYPE_VIDEO_CAVS      = 0x42, // Chinese National Standard AVS+
    STREAM_TYPE_VIDEO_AVS2      = 0xd2, // Chinese National Standard AVS2
    STREAM_TYPE_VIDEO_AVS3      = 0xd4, // Chinese National Standard AVS3
    STREAM_TYPE_VIDEO_VC1       = 0xea, // Microsoft VC1
    STREAM_TYPE_VIDEO_SVAC      = 0x80, // GBT 25724-2010 SVAC(2014)
    STREAM_TYPE_AUDIO_SVAC      = 0x9B, // GBT 25724-2010 SVAC(2014)
    STREAM_TYPE_AUDIO_G711A     = 0x90, // GBT 25724-2010 SVAC(2014)
    STREAM_TYPE_AUDIO_G711U     = 0x91,
    STREAM_TYPE_AUDIO_G722      = 0x92,
    STREAM_TYPE_AUDIO_G723      = 0x93,
    STREAM_TYPE_AUDIO_G729      = 0x99,
    STREAM_TYPE_AUDIO_OPUS      = 0x9c, // https://opus-codec.org/docs/ETSI_TS_opus-v0.1.3-draft.pdf
};
// clang-format on

// Transport packet
//
// bits field
//  8   sync_byte.
//  1   transport_error_indicator.
//  1   payload_unit_start_indicator.
//  1   transport_priority.
// 13   PID.
//  2   transport_scrambling_control.
//  2   adaptation_field_control.
//  4   continuity_counter.
//
//  if (adaptation_field_control = = '10' || adaptation_field_control = = '11'){
//      adaptation_field()
//  }
//  if(adaptation_field_control = = '01' || adaptation_field_control = = '11') {
//      for (i = 0; i < N; i++){
//        data_byte
//      }
//  }
//

struct TSPacketHeader {
    uint8_t sync_byte = TS_SYNC_BYTE;
    uint8_t pid_h : 5;
    uint8_t transport_priority : 1;
    uint8_t payload_unit_start_indicator : 1;
    uint8_t transport_error_indicator : 1;
    uint8_t pid_l : 8;
    uint8_t continuity_counter : 4;
    uint8_t adaptation_field_control : 2;
    uint8_t transport_scrambling_control : 2;
    uint8_t data[0];

    uint16_t GetPID() { return ((pid_h & 0xff) << 8) | pid_l; }
    void SetPID(uint16_t pid) {
        pid_h = pid >> 8;
        pid_l = pid & 0xff;
    }
};

// Service Description Table
//
// bits field
//  8   table_id.
//  1   section_syntax_indicator.
//  1   reserved_future_use.
//  2   reserved.
// 12   section_length.
// 16   transport_stream_id.
//  2   reserved.
//  5   version_number.
//  1   current_next_indicator.
//  8   section_number.
//  8   last_section_number
// 16   original_network_id
//  8   reserved_future_use
//
//  for (i=0;i<N;i++){
//      16  service_id
//       6  reserved_future_use
//       1  EIT_schedule_flag
//       1  EIT_present_following_flag
//       3  running_status
//       1  free_CA_mode
//      12  descriptors_loop_length
//
//      for (i = 0; i < N; i++) {
//          descriptor()
//      }
//  }
//
//  32  CRC_32
//
struct TS_Description {
    TS_TID tableId : 8; // TID_SDS 0x42, TID_SDS_O 0x46
};

// Prohibit cast type conversion
class TS_Adaption {
public:
    bool Parse(const uint8_t *data, size_t size);

public:
    uint8_t adaptation_field_length : 8;

    uint8_t discontinuity_indicator : 1;
    uint8_t random_access_indicator : 1;
    uint8_t elementary_stream_priority_indicator : 1;
    uint8_t PCR_flag : 1;
    uint8_t OPCR_flag : 1;
    uint8_t splicing_point_flag : 1;
    uint8_t transport_private_data_flag : 1;
    uint8_t adaptation_field_extension_flag : 1;

    // if (PCR_flag = = '1')
    uint64_t program_clock_reference_base : 33;
    uint16_t program_clock_reference_extension : 9;

    // if (OPCR_flag = = '1')
    uint64_t original_program_clock_reference_base : 33;
    uint16_t original_program_clock_reference_extension : 9;

    // if (splicing_point_flag = = '1')
    uint8_t splice_countdown : 8;

    // if (transport_private_data_flag = = '1')
    uint8_t transport_private_data_length : 8;

    // if (adaptation_field_extension_flag = = '1')
    uint8_t adaptation_field_extension_length : 8;
    uint8_t ltw_flag : 1;
    uint8_t piecewise_rate_flag : 1;
    uint8_t seamless_splice_flag : 1;
    uint8_t af_descriptor_not_present_flag : 1;

    // if (ltw_flag = = '1')
    uint8_t ltw_valid_flag : 1;
    uint16_t ltw_offset : 15;

    // if (piecewise_rate_flag = = '1')
    uint32_t piecewise_rate : 22;

    // if (seamless_splice_flag = = '1')
    uint8_t Splice_type : 4;
    uint32_t DTS_next_AU;
};

struct Frame {
    StreamType codecId;
    int64_t pts;
    int64_t dts;
    data_t data;
    void Clear() {
        codecId = STREAM_TYPE_RESERVED;
        pts = 0;
        dts = 0;
        data.clear();
    }
};

class TS_PES {
public:
    int Parse(const uint8_t *data, size_t size);

public:
    uint32_t packet_start_code_prefix : 24; // 0x000001
    uint8_t stream_id : 8;
    uint16_t PES_packet_length : 16;

    uint8_t : 2; // '10'
    uint8_t PES_scrambling_control : 2;
    uint8_t PES_priority : 1;
    uint8_t data_alignment_indicator : 1;
    uint8_t copyright : 1;
    uint8_t original_or_copy : 1;

    uint8_t PTS_DTS_flags : 2;
    uint8_t ESCR_flag : 1;
    uint8_t ES_rate_flag : 1;
    uint8_t DSM_trick_mode_flag : 1;
    uint8_t additional_copy_info_flag : 1;
    uint8_t PES_CRC_flag : 1;
    uint8_t PES_extension_flag : 1;

    uint8_t PES_header_data_length : 8;

    // if (PTS_DTS_flags == '10' || PTS_DTS_flags == '11')
    uint32_t PTS;
    // if (PTS_DTS_flags == '11')
    uint32_t DTS;

    // if (ESCR_flag == '1')
    uint32_t ESCR_base;

    // if (ES_rate_flag == '1')
    uint32_t ES_rate : 22;
    uint16_t ESCR_extension : 9;

    // if (DSM_trick_mode_flag == '1')
    uint8_t trick_mode_control : 3;

    uint8_t field_id : 2;
    uint8_t intra_slice_refresh : 1;
    uint8_t frequency_truncation : 2;

    uint8_t rep_cntrl : 5;

    uint8_t marker_bit : 1;
    uint8_t additional_copy_info : 7;

    uint16_t previous_PES_packet_CRC : 16;

    // if ( PES_extension_flag == '1')
    uint8_t PES_private_data_flag : 1;
    uint8_t pack_header_field_flag : 1;
    uint8_t program_packet_sequence_counter_flag : 1;
    uint8_t P_STD_buffer_flag : 1;
    uint8_t PES_extension_flag_2 : 1;

    // if ( PES_private_data_flag == '1')
    uint8_t PES_private_data[16];

    // if (pack_header_field_flag == '1')
    uint8_t pack_field_length : 8;

    // if (program_packet_sequence_counter_flag == '1')
    uint8_t program_packet_sequence_counter : 7;
    uint8_t MPEG1_MPEG2_identifier : 1;
    uint8_t original_stuff_length : 6;

    // if ( P-STD_buffer_flag == '1')
    uint8_t P_STD_buffer_scale : 1;
    uint16_t P_STD_buffer_size : 13;

    // if ( PES_extension_flag_2 == '1')
    uint8_t PES_extension_field_length : 7;
    uint8_t stream_id_extension_flag : 1;
    uint8_t stream_id_extension : 7;
    uint8_t tref_extension_flag : 1;
    uint32_t TREF : 32;

    // extra
    int have_pes_header;
    int flags;
    Frame frame;
};

struct TS_PMT_Stream {
    uint8_t stream_type : 8; // StreamType
    uint16_t elementary_PID : 16;
    uint16_t ES_info_length : 12;
    unsigned descriptor;

    uint8_t continuity_counter : 4;

    std::shared_ptr<TS_PES> pes;
};

// Transport Stream Program Map Table
class TS_PMT {
public:
    bool Parse(const uint8_t *data, size_t size);

public:
    uint8_t table_id : 8;
    uint8_t section_syntax_indicator : 1;
    uint8_t zero : 1;
    uint8_t reserved1 : 2;
    uint16_t section_length : 12; // starting following the section_length field, and including the CRC.
    uint16_t program_number : 16;
    uint8_t reserved2 : 2;
    uint8_t version_number : 5;
    uint8_t current_next_indicator : 1;
    uint8_t section_number : 8;
    uint8_t last_section_number : 8;
    uint8_t reserved3 : 3;
    uint16_t PCR_PID : 13;
    uint8_t reserved4 : 4;
    uint16_t program_info_length : 12;

    std::vector<TS_PMT_Stream> streams;
};

struct TS_PAT_Program {
    uint16_t program_number : 16;
    uint16_t program_map_PID : 13;

    std::shared_ptr<TS_PMT> pmt;
};

class TS_PAT {
public:
    bool Parse(const uint8_t *data, size_t size);

public:
    uint8_t table_id = TID_PAS;
    uint8_t section_syntax_indicator : 1;
    uint16_t section_length : 12; // starting following the section_length field, and including the CRC.
    uint16_t transport_stream_id : 16;
    uint8_t version_number : 5;
    uint8_t current_next_indicator : 1;
    uint8_t section_number : 8;
    uint8_t last_section_number : 8;

    uint32_t crc = 0;

    std::vector<TS_PAT_Program> programs;
};

#endif // MPEG_TS_MEDIA_SRC_MPEG_TS_H