# TestFFmpegAPI
A testing app which invokes FFmpeg APIs (instead of the complied ffmpeg.exe)  
Tested with FFmpeg v7.1

**How to run?**
1. go to https://ffmpeg.org/download.html#build-windows  
2. click "Windows builds from gyan.dev"  
3. under "release builds", select ffmpeg-release-full-shared.7z  
4. when the download finishes, unzip the package
5. copy everything (except .exe) in ffmpeg-7.1-full_build-shared\bin into App Build folder  

Visual Studio configuration:
1. Add extra include path,e.g. D:\SDKs\ffmpeg-7.1-full_build-shared\include
2. Add extra lib path, e.g. D:\SDKs\ffmpeg-7.1-full_build-shared\lib
3. Add libs: avcodec.lib;avdevice.lib;avformat.lib;avutil.lib;swscale.lib;swresample.lib
4. include FFmpeg header files as below:

#ifdef __cplusplus
extern "C" {
#endif
   
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif