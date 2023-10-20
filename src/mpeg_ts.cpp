//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "mpeg_ts.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

bool TS_Adaption::Parse(const uint8_t *data, size_t size) {
    assert(size <= TS_PACKET_SIZE);

    size_t i = 0;
    printf("TS_Adaption::Parse %02x %02x %02x %02x\n", data[i], data[i + 1], data[i + 2], data[i + 3]);
    adaptation_field_length = data[i++];
    if (adaptation_field_length > 0) {
        discontinuity_indicator = (data[i] >> 7) & 0x01;
        random_access_indicator = (data[i] >> 6) & 0x01;
        elementary_stream_priority_indicator = (data[i] >> 5) & 0x01;
        PCR_flag = (data[i] >> 4) & 0x01;
        OPCR_flag = (data[i] >> 3) & 0x01;
        splicing_point_flag = (data[i] >> 2) & 0x01;
        transport_private_data_flag = (data[i] >> 1) & 0x01;
        adaptation_field_extension_flag = data[i] & 0x01;
        i++;

        if (PCR_flag) {
            assert(i + 6 <= size);
            program_clock_reference_base = ((uint64_t)data[i] << 25) | ((uint64_t)data[i + 1] << 17) |
                                           ((uint64_t)data[i + 2] << 9) | ((uint64_t)data[i + 3] << 1) |
                                           ((data[i + 4] >> 7) & 0x01);
            program_clock_reference_extension = ((data[i + 4] & 0x01) << 8) | data[i + 5];
            i += 6;
        }

        if (OPCR_flag) {
            assert(i + 6 <= size);
            original_program_clock_reference_base = (((uint64_t)data[i]) << 25) | ((uint64_t)data[i + 1] << 17) |
                                                    ((uint64_t)data[i + 2] << 9) | ((uint64_t)data[i + 3] << 1) |
                                                    ((data[i + 4] >> 7) & 0x01);
            original_program_clock_reference_extension = ((data[i + 4] & 0x01) << 8) | data[i + 5];
            i += 6;
        }

        if (splicing_point_flag) {
            splice_countdown = data[i++];
        }

        if (transport_private_data_flag) {
            transport_private_data_length = data[i++];
            assert(i + transport_private_data_length <= size);
            for (uint8_t j = 0; j < transport_private_data_length; j++) {
                uint8_t transport_private_data = data[i + j];
            }

            i += transport_private_data_length;
        }

        if (adaptation_field_extension_flag) {
            adaptation_field_extension_length = data[i++];
            ltw_flag = (data[i] >> 7) & 0x01;
            piecewise_rate_flag = (data[i] >> 6) & 0x01;
            seamless_splice_flag = (data[i] >> 5) & 0x01;
            af_descriptor_not_present_flag = (data[i] >> 4) & 0x01;
            uint8_t reserved = data[i] & 0x0f;

            i++;
            if (ltw_flag) {
                assert(i + 2 <= size);
                ltw_valid_flag = (data[i] >> 7) & 0x01;
                ltw_offset = ((data[i] & 0x7F) << 8) | data[i + 1];
                i += 2;
            }

            if (piecewise_rate_flag) {
                assert(i + 3 <= size);
                piecewise_rate = ((data[i] & 0x3f) << 16) | (data[i + 1] << 8) | data[i + 2];
                i += 3;
            }

            if (seamless_splice_flag) {
                assert(i + 5 <= size);
                Splice_type = (data[i] >> 4) & 0x0F;
                DTS_next_AU = (((data[i] >> 1) & 0x07) << 30) | (data[i + 1] << 22) |
                              (((data[i + 2] >> 1) & 0x7F) << 15) | (data[i + 3] << 7) | ((data[i + 4] >> 1) & 0x7F);

                i += 5;
            }

            // others
        }
    }

    return true;
}

int TS_PES::Parse(const uint8_t *data, size_t size) {

    printf("TS_PES::Parse() %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
    size_t i = 0;
    packet_start_code_prefix = (data[0] << 16) | (data[1] << 8) | data[2];
    stream_id = data[3];
    PES_packet_length = (data[4] << 8) | data[5]; // may be 0
    // assert(packet_start_code_prefix == 0x000001);
    assert(data[0] == 0 && data[1] == 0 && data[2] == 0x01);

    i = 6;
    assert(((data[i] >> 6) & 0x03) == 0x02);
    PES_scrambling_control = (data[i] >> 4) & 0x03;
    PES_priority = (data[i] >> 3) & 0x01;
    data_alignment_indicator = (data[i] >> 2) & 0x01;
    copyright = (data[i] >> 1) & 0x01;
    original_or_copy = data[i++] & 0x01;

    PTS_DTS_flags = (data[i] >> 6) & 0x03;
    ESCR_flag = (data[i] >> 5) & 0x01;
    ES_rate_flag = (data[i] >> 4) & 0x01;
    DSM_trick_mode_flag = (data[i] >> 3) & 0x01;
    additional_copy_info_flag = (data[i] >> 2) & 0x1;
    PES_CRC_flag = (data[i] >> 1) & 0x01;
    PES_extension_flag = data[i++] & 0x01;

    PES_header_data_length = data[i++];
    if (PTS_DTS_flags & 0x02) {
        assert(0x20 == (data[i] & 0x20));
        PTS = ((((uint64_t)data[i] >> 1) & 0x07) << 30) | ((uint64_t)data[i + 1] << 22) |
              ((((uint64_t)data[i + 2] >> 1) & 0x7f) << 15) | ((uint64_t)data[i + 3] << 7) |
              ((data[i + 4] >> 1) & 0x7f);

        i += 5;
    }

    if (PTS_DTS_flags & 0x01) {
        assert(0x10 == (data[i] & 0x10));
        DTS = ((((uint64_t)data[i] >> 1) & 0x07) << 30) | ((uint64_t)data[i + 1] << 22) |
              ((((uint64_t)data[i + 2] >> 1) & 0x7f) << 15) | ((uint64_t)data[i + 3] << 7) |
              ((data[i + 4] >> 1) & 0x7f);
        i += 5;
    } else {
        if (PTS_DTS_flags & 0x02) {
            DTS = PTS;
        }
    }

    if (ESCR_flag == 0x01) {
        ESCR_base = ((((uint64_t)data[i] >> 3) & 0x07) << 30) | (((uint64_t)data[i] & 0x03) << 28) |
                    ((uint64_t)data[i + 1] << 20) | ((((uint64_t)data[i + 2] >> 3) & 0x1f) << 15) |
                    (((uint64_t)data[i + 2] & 0x3) << 13) | ((uint64_t)data[i + 3] << 5) | ((data[i + 4] >> 3) & 0x1f);
        ESCR_extension = ((data[i + 4] & 0x03) << 7) | ((data[i + 5] >> 1) & 0x7F);
        i += 6;
    }

    if (ES_rate_flag) {
        ES_rate = ((data[i] & 0x7f) << 15) | (data[i + 1] << 7) | ((data[i + 2] >> 1) & 0x7f);
        i += 3;
    }

    if (DSM_trick_mode_flag == 0x01) {
        trick_mode_control = (data[i] >> 5) & 0x07;
        i += 1;
    }

    if (additional_copy_info_flag == 0x01) {
        additional_copy_info = data[i] & 0x7f;
        i += 1;
    }

    if (PES_CRC_flag == 0x01) {
        previous_PES_packet_CRC = (data[i] << 8) | data[i + 1];
        i += 2;
    }

    if (PES_extension_flag == 0x01) {
    }

    return PES_header_data_length + 9; // size before 'PES_header_data_length'
}

bool TS_PMT::Parse(const uint8_t *data, size_t size) {
    int crc32Size = 4;

    assert(size > 12 + crc32Size);

    // 12 bytes fiexed header
    table_id = data[0];
    section_syntax_indicator = (data[1] >> 7) & 0x01;
    section_length = ((data[1] & 0x0f) << 8) | data[2];
    program_number = (data[3] << 8) | data[4];
    version_number = (data[5] >> 1) & 0x1f;
    current_next_indicator = data[5] & 0x01;
    section_number = data[6];
    last_section_number = data[7];

    PCR_PID = ((data[8] & 0x1f) << 8) | data[9];
    program_info_length = ((data[10] & 0x0f) << 8) | data[11];

    assert(table_id == TID_PMS);
    assert(section_syntax_indicator == 1);
    assert(section_number == 0);
    assert(last_section_number == 0);

    int totalSizeOfPMT = section_length + 3;
    assert(totalSizeOfPMT > 12 + crc32Size);
    assert(totalSizeOfPMT <= size);
    assert(program_info_length + 12 <= totalSizeOfPMT - crc32Size);

    if (program_info_length > 0) {
        // descriptor(data + 12, program_info_length)
    }

    size_t i = 0;
    uint16_t ES_info_length = 0;
    for (i = 12 + program_info_length; i + 5 <= totalSizeOfPMT - crc32Size && totalSizeOfPMT <= size;
         i += ES_info_length + 5) {
        uint8_t stream_type = data[i];
        uint16_t elementary_PID = ((data[i + 1] & 0x1f) << 8) | data[i + 2];
        ES_info_length = ((data[i + 3] & 0x0f) << 8) | data[i + 4];

        if (i + 5 + ES_info_length > totalSizeOfPMT - crc32Size) {
            break;
        }

        auto it = std::find_if(streams.begin(), streams.end(),
                               [&](const TS_PMT_Stream &stream) { return stream.elementary_PID == elementary_PID; });

        if (it == streams.end()) {
            TS_PMT_Stream stream;
            stream.stream_type = stream_type; // codec id
            stream.elementary_PID = elementary_PID;
            stream.ES_info_length = ES_info_length;
            if (ES_info_length > 0) {
                // descriptor(data + i + 5, ES_info_length)
            }

            printf("Find stream_type: 0x%02x, pid: 0x%04x | refer to 0x1b: AVC; 0x0f: AAC; 0x24: HEVC\n", stream_type,
                   elementary_PID);
            streams.emplace_back(stream);
        } else {
            if (it->stream_type != stream_type) {
                it->stream_type = stream_type; // codec id
                it->continuity_counter = 0x0f;
                it->pes.reset();
            }
            it->ES_info_length = ES_info_length;
        }
    }

    assert(i + 4 <= size);
    printf("%02x %02x %02x %02x\n", data[i], data[i + 1], data[i + 2], data[i + 3]);
    uint32_t crc = (data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3];
    // check crc32
    return true;
}

bool TS_PAT::Parse(const uint8_t *data, size_t size) {
    size_t i = 0;
    int crc32Size = 4;

    // 8 bytes fiexed header
    table_id = data[0];
    section_syntax_indicator = (data[1] >> 7) & 0x01;
    section_length = ((data[1] & 0x0f) << 8) | data[2];
    transport_stream_id = (data[3] << 8) | data[4];
    version_number = (data[5] >> 1) & 0x1f;
    current_next_indicator = data[5] & 0x01;
    section_number = data[6];
    last_section_number = data[7];

    assert(table_id == TID_PAS);
    assert(8 + crc32Size <= size);
    int totalSizeOfPAT = section_length + 3;
    assert(totalSizeOfPAT >= 8 + crc32Size); // PAT size = section_length +3

    printf("section_length: %d\n", section_length);
    for (i = 8; i + 4 <= totalSizeOfPAT - 4 && section_length + 3 <= size; i += 4) {
        printf("%02x %02x %02x %02x\n", data[i], data[i + 1], data[i + 2], data[i + 3]);

        uint16_t program_number = (data[i] << 8) | data[i + 1];
        if (program_number == 0) {
            uint16_t network_PID = ((data[i + 2] & 0x1f) << 8) | data[i + 3];
        } else {
            uint16_t program_map_PID = ((data[i + 2] & 0x1f) << 8) | data[i + 3];

            auto it = std::find_if(programs.begin(), programs.end(), [&](const TS_PAT_Program &program) {
                return program.program_map_PID == program_map_PID;
            });

            if (it == programs.end()) {
                TS_PAT_Program program;
                program.program_number = program_number;
                program.program_map_PID = program_map_PID;
                programs.emplace_back(program);
                printf("program_number: %d, program_map_PID:0x%02x\n", program.program_number, program.program_map_PID);
            } else {
                it->program_number = program_number;
                printf("find program, program_number: %d, program_map_PID:0x%02x\n", it->program_number,
                       it->program_map_PID);
            }
        }
    }

    printf("i = %ld, size = %zu\n", i, size);
    assert(i + 4 < size);
    printf("%02x %02x %02x %02x\n", data[i], data[i + 1], data[i + 2], data[i + 3]);
    crc = (data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3];
    // check crc32
    return true;
}