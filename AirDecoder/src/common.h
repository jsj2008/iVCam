
#ifndef _INSUTIL_COMMON_H_
#define _INSUTIL_COMMON_H_

#define CM_PIX_FMT_YUV420P 1
#define CM_PIX_FMT_NV12    2
#define CM_PIX_FMT_RGB24   3
#define CM_PIX_FMT_BGR24   4
#define CM_PIX_FMT_RGBA    5
#define CM_PIX_FMT_BGRA    6
#define CM_PIX_FMT_YUV422P 7

#define CM_SAMPLE_FMT_NONE  -1
#define CM_SAMPLE_FMT_s16      1
#define CM_SAMPLE_FMT_s16p    2
#define CM_SAMPLE_FMT_f32      3
#define CM_SAMPLE_FMT_f32p     4

#define CM_CH_LAYOUT_NONE    -1
#define CM_CH_LAYOUT_MONO    1
#define CM_CH_LAYOUT_STEREO  2

#define CM_DATA_POINTERS 8 

#define CM_NO_PTS_VALUE 0X8000000000000000

enum
{
    CODEC_OK = 0,                         //成功
    CODEC_ERR,                            //其他错误
    CODEC_ERR_OVER,                       //文件读完
    CODEC_ERR_WRITE_FILE_FAIL,            //写文件失败
    CODEC_ERR_OPEN_FILE_FAIL,             //打开文件失败
    CODEC_ERR_ENCODE_ERROR,               //编码失败
    CODEC_ERR_DECODE_ERROR,               //解码失败
    CODEC_ERR_GET_AVINFO_FAIL,            //获取音视频信息失败
    CODEC_ERR_VIDEO_NOT_H264,             //视频不是H264编码
    CODEC_ERR_AUDIO_NOT_AAC,              //音频不是AAC编码
    CODEC_ERR_NO_H264_ENCODER,            //找不到H264编码器
    CODEC_ERR_NO_AAC_ENCODER,             //找不到AAC编码器
    CODEC_ERR_CREATE_STREAM_FAIL,         //创建流失败
    CODEC_ERR_MALLOC_FAIL,                //分配内存失败
    CODEC_ERR_OPEN_ENCODE_FAIL,           //打开编码器失败
    CODEC_ERR_OPEN_DECODE_FAIL,           //打开解码器失败
};

struct EncodeFrame
{
    ~EncodeFrame();
    
    unsigned char* data = nullptr;
    unsigned int len = 0;
    long long pts = 0;
    long long dts = 0;
    bool is_keyframe = false;
    
    void* private_data = nullptr;
};

struct DecodeFrame
{
    ~DecodeFrame();
    
    unsigned char* data[CM_DATA_POINTERS] = {nullptr};
    int linesize[CM_DATA_POINTERS] = {0};
    long long pts = 0;
    long long dts = 0;
    
    void* private_data = nullptr;
    unsigned char private_type = 0;
};

struct DecodeFrame2
{
    ~DecodeFrame2();
    unsigned char* data = nullptr;
    unsigned int len = 0;
    long long pts = 0;
    long long dts = 0;
    
    void* private_data = nullptr;
    unsigned char private_type = 0;
};

#endif
