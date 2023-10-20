//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "mpeg_ts_demuxer.h"
#include "mpeg_ts.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

static uint64_t count = 0;
void MpegTsDemuxer::Input(const uint8_t *data, size_t size) {
    printf("[%lu] Input data %02x %02x %02x %02x, size: %zu\n", count++, data[0], data[1], data[2], data[3], size);

    assert(size == TS_PACKET_SIZE);
    assert(data[0] == TS_SYNC_BYTE);

    TSPacketHeader *tsPacket = (TSPacketHeader *)data;

    uint16_t pid = tsPacket->GetPID();
    printf("PID[0x%04x]: Error: %u, Start:%u, Priority:%u, Scrambler:%u, Adaptation: %u, Counter: %u\n", pid,
           tsPacket->transport_error_indicator, tsPacket->payload_unit_start_indicator, tsPacket->transport_priority,
           tsPacket->transport_scrambling_control, tsPacket->adaptation_field_control, tsPacket->continuity_counter);

    size_t i = 4;
    if (tsPacket->adaptation_field_control & 0x02) {
        printf("Find adaptation field\n");
        TS_Adaption adaptation;
        adaptation.Parse(data + i, size - i);

        if (adaptation.adaptation_field_length > 0 && adaptation.PCR_flag) {
            int64_t t = adaptation.program_clock_reference_base / 90L; // ms
            printf("pcr: %02d:%02d:%02d.%03d - %lu/%u\n", (int)(t / 3600000), (int)(t % 3600000) / 60000,
                   (int)((t / 1000) % 60), (int)(t % 1000), adaptation.program_clock_reference_base,
                   adaptation.program_clock_reference_extension);
        }

        i += (adaptation.adaptation_field_length + 1);
        if (i + (tsPacket->payload_unit_start_indicator ? 1 : 0) >= size) {
            return;
        }
    }

    if (tsPacket->adaptation_field_control & 0x01) {
        if (pid == PID_PAT) {
            printf("This is a PAT\n");
            if (tsPacket->payload_unit_start_indicator) {
                i++; // skip pointer_field 0x00
            }

            pat_.Parse(data + i, size - i);
        } else if (pid == PID_SDT) {
            HandleSDT(data, size);
        } else {
            for (size_t j = 0; j < pat_.programs.size(); j++) {
                if (pid == pat_.programs[j].program_map_PID) {
                    printf("This is a PMT\n");
                    if (tsPacket->payload_unit_start_indicator) {
                        i++;
                    }

                    if (!pat_.programs[j].pmt) {
                        pat_.programs[j].pmt = std::make_shared<TS_PMT>();
                    }
                    pat_.programs[j].pmt->Parse(data + i, size - i);
                    break;
                } else {
                    for (size_t k = 0; k < pat_.programs[j].pmt->streams.size(); k++) {
                        TS_PMT_Stream *stream = &pat_.programs[j].pmt->streams[k];
                        if (pid != stream->elementary_PID) {
                            printf("stream->elementary_PID: %d, current PID: %d\n", stream->elementary_PID, pid);
                            continue;
                        }

                        printf("Find stream with pid: %04x\n", pid);

                        if (!stream->pes) {
                            stream->pes = std::make_shared<TS_PES>();
                            stream->continuity_counter = 0x0f;
                        }

                        if (tsPacket->continuity_counter != (stream->continuity_counter + 1) % 16) {
                            printf("Error pes lost, lastCC = %d, currentCC = %d\n", stream->continuity_counter,
                                   tsPacket->continuity_counter);
                        }
                        stream->continuity_counter = tsPacket->continuity_counter;

                        if (tsPacket->payload_unit_start_indicator) {
                            size_t n = stream->pes->Parse(data + i, size - i);
                            assert(n > 0);
                            i += n;
                            printf("payload_unit_start_indicator i = %zu, n = %zu\n", i, n);
                            stream->pes->have_pes_header = n > 0 ? 1 : 0;
                        } else if (!stream->pes->have_pes_header) {
                            continue; // don't have pes header yet
                        }

                        const uint8_t *p = data + i;
                        size_t length = size - i;

                        assert(stream->pes->DTS != 0);

                        if (length < 0) {
                            continue;
                        }

                        if (tsPacket->payload_unit_start_indicator || stream->pes->DTS != stream->pes->frame.dts) {
                            if (callback_ && stream->pes->frame.data.size()) {
                                printf("callback\n");
                                callback_(stream->pes->frame.codecId, stream->pes->frame.pts, stream->pes->frame.dts,
                                          (const uint8_t *)stream->pes->frame.data.data(),
                                          stream->pes->frame.data.size());
                            }
                            stream->pes->frame.Clear();
                            stream->pes->frame.dts = stream->pes->DTS;
                            stream->pes->frame.pts = stream->pes->PTS;
                            stream->pes->frame.codecId = (StreamType)stream->stream_type;
                        }

                        printf("append frame (%zu)\n", length);
                        stream->pes->frame.data.append((const char *)p, length);

                        break; // find stream
                    }
                }
            }
        }
    }
}

void MpegTsDemuxer::Flush() {
    printf("Flush() enter\n");
    for (size_t i = 0; i < pat_.programs.size(); i++) {
        for (size_t j = 0; j < pat_.programs[i].pmt->streams.size(); j++) {
            TS_PMT_Stream *stream = &pat_.programs[i].pmt->streams[j];
            if (stream && stream->pes) {
                if (callback_ && stream->pes->frame.data.size()) {
                    printf("callback\n");
                    callback_(stream->pes->frame.codecId, stream->pes->frame.pts, stream->pes->frame.dts,
                              (const uint8_t *)stream->pes->frame.data.data(), stream->pes->frame.data.size());
                }
                stream->pes->frame.Clear();
            }
        }
    }
}

void MpegTsDemuxer::HandleSDT(const uint8_t *data, size_t size) {
    printf("Handle SDT\n");
}