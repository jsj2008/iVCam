extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

#include <mutex>
#include "util.h"

void ffRegAll()
{
    static std::mutex mtx;
    
    mtx.lock();
    
    av_register_all();
    
    mtx.unlock();
}

void ffRegAllCodec()
{
    static std::mutex mtx;
    
    mtx.lock();
    
    avcodec_register_all();
    
    mtx.unlock();
}

void ffNetworkInit()
{
    static std::mutex mtx;
    
    mtx.lock();
    
    avformat_network_init();
    
    mtx.unlock();
}

std::string fferr2str(int err)
{
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    
    return std::string(av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, err));
}

int Convert2FFPixFmt(unsigned char pix_fmt)
{
    switch (pix_fmt)
    {
        case CM_PIX_FMT_YUV420P:
            return AV_PIX_FMT_YUV420P;
            
        case CM_PIX_FMT_YUV422P:
            return AV_PIX_FMT_YUV422P;
            
        case CM_PIX_FMT_NV12:
            return AV_PIX_FMT_NV12;
            
        case CM_PIX_FMT_RGB24:
            return AV_PIX_FMT_RGB24;
            
        case CM_PIX_FMT_BGR24:
            return AV_PIX_FMT_BGR24;
            
        case CM_PIX_FMT_RGBA:
            return AV_PIX_FMT_RGBA;

        case CM_PIX_FMT_BGRA:
            return AV_PIX_FMT_BGRA;

        default:
            return AV_PIX_FMT_NONE;
    }
}

EncodeFrame::~EncodeFrame()
{
    if (private_data)
    {
        av_free_packet((AVPacket*)private_data);
        delete (AVPacket*)private_data;
        private_data = nullptr;
        data = nullptr;
        len = 0;
    }
}

DecodeFrame::~DecodeFrame()
{
    if (!private_data)
    {
        return;
    }
    
    switch (private_type)
    {
        case DEC_PRI_DATA_TYPE_PIC:
        {
            AVPicture* pic = (AVPicture*)private_data;
            avpicture_free(pic);
            delete pic;
            break;
        }
        case DEC_PRI_DATA_TYPE_FRAME:
        {
            AVFrame* frame = (AVFrame*)private_data;
            av_frame_unref(frame);
            av_frame_free(&frame);
            break;
        }
        case DEC_PRI_DATA_TYPE_ARRY:
        {
            delete[] (unsigned char*)private_data;
            break;
        }
            
        default:
            break;
    }
    
    private_data = nullptr;
}

DecodeFrame2::~DecodeFrame2()
{
    if (!private_data)
    {
        return;
    }
    
    switch (private_type)
    {
        case DEC_PRI_DATA_TYPE_PIC:
        {
            AVPicture* pic = (AVPicture*)private_data;
            avpicture_free(pic);
            delete pic;
            break;
        }
        case DEC_PRI_DATA_TYPE_FRAME:
        {
            AVFrame* frame = (AVFrame*)private_data;
            av_frame_unref(frame);
            av_frame_free(&frame);
            break;
        }
        case DEC_PRI_DATA_TYPE_ARRY:
        {
            delete[] (unsigned char*)private_data;
            break;
        }
            
        default:
            break;
    }
    
    private_data = nullptr;
}
