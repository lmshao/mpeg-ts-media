//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include <cstdint>
#include <memory>
#include <stdio.h>

#include "file.h"
#include "mpeg_ts.h"
#include "mpeg_ts_demuxer.h"

int main(int argc, char **argv) {
    printf("MPEG-TS demuxer tool\n");
    if (argc != 2) {
        printf("Miss parameter, please specify a file.\n");
        return -1;
    }

    std::shared_ptr<FileWriter> avcFile;
    std::shared_ptr<FileWriter> aacFile;
    std::shared_ptr<FileWriter> hevcFile;
    std::shared_ptr<FileWriter> mpegAudioFile;

    MpegTsDemuxer demuxer;
    demuxer.SetDemuxCallback([&](StreamType codec, int64_t pts, int64_t dts, const uint8_t *data, size_t size) {
        printf("StreamType: 0x%02x, pts: %ld, dts: %ld, data: %02x %02x %02x %02x %02x, size: %zu\n", codec, pts, dts,
               data[0], data[1], data[2], data[3], data[4], size);

        switch (codec) {
            case STREAM_TYPE_VIDEO_H264:
                static std::shared_ptr<FileWriter> avcFile =
                    FileWriter::Open("video-" + std::to_string(time(nullptr)) + ".h264");
                avcFile->Write(data, size);
                break;
            case STREAM_TYPE_VIDEO_HEVC:
                static std::shared_ptr<FileWriter> hevcFile =
                    FileWriter::Open("video-" + std::to_string(time(nullptr)) + ".h265");
                hevcFile->Write(data, size);
                break;
            case STREAM_TYPE_AUDIO_AAC:
                static std::shared_ptr<FileWriter> aacFile =
                    FileWriter::Open("audio-" + std::to_string(time(nullptr)) + ".aac");
                aacFile->Write(data, size);
                break;
            case STREAM_TYPE_AUDIO_MPEG1:
            case STREAM_TYPE_AUDIO_MPEG2:
                static std::shared_ptr<FileWriter> mpegAudioFile;
                if (!mpegAudioFile) {
                    uint8_t layer = (data[1] >> 1) & 0x02;
                    std::string suffix(".mp3");
                    if (layer == 0b01) {
                        suffix = ".mp3";
                    } else if (layer == 0b10) {
                        suffix = ".mp2";
                    } else if (layer == 0b11) {
                        suffix = ".mp1";
                    }

                    mpegAudioFile = FileWriter::Open("audio-" + std::to_string(time(nullptr)) + suffix);
                }
                mpegAudioFile->Write(data, size);
                break;
            case STREAM_TYPE_VIDEO_MPEG1:
                static std::shared_ptr<FileWriter> mpeg1File =
                    FileWriter::Open("video-" + std::to_string(time(nullptr)) + ".mpeg1video");
                mpeg1File->Write(data, size);
                break;
            case STREAM_TYPE_VIDEO_MPEG2:
                static std::shared_ptr<FileWriter> mpeg2File =
                    FileWriter::Open("video-" + std::to_string(time(nullptr)) + ".mpeg2video");
                mpeg2File->Write(data, size);
                break;
            default:
                printf("Unsupported stream type 0x%02x\n", codec);
                break;
        }
    });

    int count = 0;
    auto file = FileReader::Open(argv[1]);
    uint8_t *end = file->data + file->size;
    uint8_t *p = file->data;
    while (p + TS_PACKET_SIZE <= end) {
        if (p[0] == 0x47) {
            if (p + TS_PACKET_SIZE == end || p[TS_PACKET_SIZE] == 0x47) {
                demuxer.Input(p, TS_PACKET_SIZE);
                p += TS_PACKET_SIZE;
                continue;
            } else {
                printf("Invalid packet size\n");
            }

        } else {
            printf("Error\n");

            p++;
        }
    }
    demuxer.Flush();

    return 0;
}