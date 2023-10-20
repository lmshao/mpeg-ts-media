# mpeg-ts-media
MPEG-TS (.ts) file format parsing tool.   
Currently, only the demultiplexing (`*.ts` -> `*.h264|*.aac|*.h265 .etc`) is implemented.

## Usage

```sh
mkdir build && cd build
cmake ..
make
./ts_media ../sample.ts > log 
```
Several raw audio and video files will be generated in the current directory.
