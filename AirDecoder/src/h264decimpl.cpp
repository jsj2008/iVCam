
#include <string>
#include "h264decimpl.h"
#include "common.h"
#include "log.h"

int H264DecImpl::Open(H264DecParam& param)
{
	LOGINFO("H264DEC width:%u height:%u threads:%u spslen:%u ppslen:%u", param.width, param.height, param.threads, param.sps_len, param.pps_len);
    
    pix_fmt_ = (AVPixelFormat)Convert2FFPixFmt(param.pix_fmt);

	ffRegAllCodec();

	codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec_)
	{
		LOGERR("avcodec_find_decoder fail");
		return CODEC_ERR_NO_H264_ENCODER;
	}

	codec_ctx_ = avcodec_alloc_context3(codec_);
	if (!codec_ctx_)
	{
		LOGERR("avcodec_alloc_context fail");
		return CODEC_ERR;
	}

	codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_ctx_->codec_id = AV_CODEC_ID_H264;
	codec_ctx_->thread_count = param.threads;
	codec_ctx_->refcounted_frames = 1; 
    
    codec_ctx_->extradata_size = param.sps_len + param.pps_len + 5 + 3 * 2;
    codec_ctx_->extradata = new unsigned char[codec_ctx_->extradata_size]();
    
    codec_ctx_->extradata[0] = 1;
    codec_ctx_->extradata[4] = 0xff;
    codec_ctx_->extradata[5] = 0xe1;
    codec_ctx_->extradata[7] = param.sps_len;
    memcpy(codec_ctx_->extradata+8, param.sps, param.sps_len);
    codec_ctx_->extradata[8+param.sps_len] = 1;
    codec_ctx_->extradata[8+param.sps_len+2] = param.pps_len;
    memcpy(codec_ctx_->extradata+param.sps_len+8+3, param.pps, param.pps_len);
    
	int ret = avcodec_open2(codec_ctx_, codec_, nullptr);
	if (ret < 0)
	{
		LOGERR("avcodec_open fail ret:%d %s", ret, FFERR2STR(ret));
		return CODEC_ERR_OPEN_DECODE_FAIL;
	}
    LOGINFO("Open decoder successfully.");
    
	return CODEC_OK;
}

void H264DecImpl::Close()
{	
	LOGINFO("H264 decoder closed.");
	
	if (codec_ctx_)
	{
        if (codec_ctx_->extradata)
        {
            delete[] codec_ctx_->extradata;
            codec_ctx_->extradata = nullptr;
        }
        
		avcodec_close(codec_ctx_);
    	av_free(codec_ctx_);
        codec_ctx_ = nullptr;
	}

	if (sw_ctx_)
	{
		sws_freeContext(sw_ctx_);
        sw_ctx_ = nullptr;
	}
}

void H264DecImpl::FlushBuff()
{
    if (codec_ctx_)
    {
        avcodec_flush_buffers(codec_ctx_);
    }
}

//flush: data = nullptr
std::shared_ptr<DecodeFrame> H264DecImpl::Decode(unsigned char* data, unsigned int len, long long pts, long long dts)
{
	int got_frame = 0;
	int ret = 0;
    
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.size = len;
    pkt.data = data;
	pkt.pts = pts;
	pkt.dts = dts;
    
    AVFrame* frame = av_frame_alloc();

	ret = avcodec_decode_video2(codec_ctx_, frame, &got_frame, &pkt);
	if (ret < 0)
	{
        av_frame_free(&frame);
		LOGERR("Error while decoding frame,ret:%d %s", ret, FFERR2STR(ret));
		return nullptr;
	}

	if (!got_frame)
	{
        av_frame_free(&frame);
        LOGERR("Got empty frame!")
		return nullptr;
	}
    
    if (!sw_ctx_ && pix_fmt_ != frame->format)
    {
        sw_ctx_ = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, frame->width, frame->height, pix_fmt_, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if(!sw_ctx_)
        {
            LOGERR("sws_getContext fail");
            return nullptr;
        }
    }
    
    auto decode_frame = std::make_shared<DecodeFrame>();
    
    decode_frame->pts = frame->pkt_pts;
    decode_frame->dts = frame->pkt_dts;
    width_ = frame->width;
    height_ = frame->height;
    
    if (pix_fmt_ != frame->format)
    {
        LOGINFO("Convert color space for frame! Width: %d, Height: %d", width_, height_);
        AVPicture* avpic = new AVPicture;
        avpicture_alloc(avpic, pix_fmt_, width_, height_);
        
        sws_scale(sw_ctx_, frame->data,frame->linesize, 0, height_, avpic->data, avpic->linesize);
        
        av_frame_unref(frame);
        av_frame_free(&frame);
        
        memcpy(decode_frame->data, avpic->data, sizeof(avpic->data));
        memcpy(decode_frame->linesize, avpic->linesize, sizeof(avpic->linesize));
        decode_frame->private_type = DEC_PRI_DATA_TYPE_PIC;
        decode_frame->private_data = (void*)avpic;
    }
    else
    {
        memcpy(decode_frame->data, frame->data, sizeof(frame->data));
        memcpy(decode_frame->linesize, frame->linesize, sizeof(frame->linesize));
        decode_frame->private_type = DEC_PRI_DATA_TYPE_FRAME;
        decode_frame->private_data = (void*)frame;
    }
    
    return decode_frame;
}

std::shared_ptr<DecodeFrame2> H264DecImpl::Decode2(unsigned char* data, unsigned int len, long long pts, long long dts)
{
    auto frame = Decode(data, len, pts, dts);
    if (!frame)
    {
        LOGERR("Failed to decode frame...");
        return nullptr;
    }
    
    auto decode_frame = std::make_shared<DecodeFrame2>();
    decode_frame->pts = frame->pts;
    decode_frame->dts = frame->dts;
    
    switch (pix_fmt_)
    {
        case AV_PIX_FMT_YUV420P:
        {
            LOGINFO("Pixel format is YUV420P.");
            decode_frame->data = new unsigned char[width_*height_*3/2]();
    
            unsigned int offset = 0;
            unsigned int i;

            /* Y */
            for (i=0; i < height_; i++)
            {   
                memcpy(decode_frame->data+offset, frame->data[0] + i * frame->linesize[0], width_);
                offset += width_;
            } 

            /* U */
            for (i=0; i < height_/2; i++)
            {   
                memcpy(decode_frame->data+offset, frame->data[1] + i * frame->linesize[1], width_/2);
                offset += width_/2;
            }

            /* V */
            for (i=0; i < height_/2; i++)
            {   
                memcpy(decode_frame->data+offset, frame->data[2] + i * frame->linesize[2], width_/2);
                offset += width_/2;
            }
 
            decode_frame->private_type = DEC_PRI_DATA_TYPE_ARRY;
            decode_frame->private_data = decode_frame->data;
            decode_frame->len = width_*height_*3/2;
            frame->private_data = nullptr;
            
            break;
        }
        case AV_PIX_FMT_YUV422P:
        {
            LOGINFO("Pixel format is YUV422P.");
            decode_frame->data = new unsigned char[width_*height_*2]();
            
            unsigned int offset = 0;
            unsigned int i;
            
            /* Y */
            for (i=0; i < height_; i++)
            {
                memcpy(decode_frame->data+offset, frame->data[0] + i * frame->linesize[0], width_);
                offset += width_;
            }
            
            /* U */
            for (i=0; i < height_; i++)
            {
                memcpy(decode_frame->data+offset, frame->data[1] + i * frame->linesize[1], width_/2);
                offset += width_/2;
            }
            
            /* V */
            for (i=0; i < height_; i++)
            {
                memcpy(decode_frame->data+offset, frame->data[2] + i * frame->linesize[2], width_/2);
                offset += width_/2;
            }
            
            decode_frame->private_type = DEC_PRI_DATA_TYPE_ARRY;
            decode_frame->private_data = decode_frame->data;
            decode_frame->len = width_*height_*2;
            frame->private_data = nullptr;
            
            break;
        }
            
        case AV_PIX_FMT_BGR24:
        case AV_PIX_FMT_RGB24:
        case AV_PIX_FMT_RGBA:
        case AV_PIX_FMT_BGRA:
        {
            decode_frame->data = frame->data[0];
            decode_frame->private_type = frame->private_type;
            decode_frame->private_data = frame->private_data;
            frame->private_data = nullptr;
            break;
        }
            
        default:
            return nullptr;
    }
    
    return decode_frame;
}
//
////                        FILE* file = fopen("/Users/zhangzhongke/Documents/out.bin", "wb");
////                        fwrite(decodedFrame->data, 1, decodedFrame->len, file);
////                        fclose(file);
//// Convert to RGB24 from YUV422P
//
//
//// Blend the image
//static BlenderParams params;
//params.input_width = 2560;
//params.input_height = 1280;
//switch (frameSize)
//{
//        // YUV422P: 1472x736x2
//    case 2166784:
//    {
//        params.output_width = 1472;
//        params.output_height = 736;
//        break;
//    }
//        // YUV422P: 2176x1088x2
//    case 4734976:
//    {
//        params.output_width = 2176;
//        params.output_height = 1088;
//        break;
//    }
//        // YUV422P: 3008x1504x2
//    case 9048064:
//    {
//        params.output_width = 3008;
//        params.output_height = 1504;
//        break;
//    }
//}
//params.input_data = nullptr;
//params.output_data = nullptr;
//params.offset = mOffset;
//
//if (mBlender)
//{
//    mBlender->runImageBlender(params, CBlenderWrapper::PANORAMIC_BLENDER);
//}
//
//// Convert back to YUV422P from RGB24

