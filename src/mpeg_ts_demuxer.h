//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef MPEG_TS_MEDIA_SRC_MPEG_TS_DEMUXER_H
#define MPEG_TS_MEDIA_SRC_MPEG_TS_DEMUXER_H

#include "mpeg_ts.h"
#include <cstdint>
#include <functional>

class MpegTsDemuxer {
public:
    using DemuxCallback =
        std::function<void(StreamType codec, int64_t pts, int64_t dts, const uint8_t *data, size_t size)>;

    void Input(const uint8_t *data, size_t size);
    void Flush();
    void SetDemuxCallback(DemuxCallback callback) { callback_ = std::move(callback); }

private:
   void HandleSDT(const uint8_t *data, size_t size); 
private:
    uint16_t pmtId_ = 0xffff;
    DemuxCallback callback_;
    TS_PAT pat_;
};

#endif // MPEG_TS_MEDIA_SRC_MPEG_TS_DEMUXER_H
