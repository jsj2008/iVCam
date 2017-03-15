
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
    
    // The biggest resolution
    blendedImage = new unsigned char[3008*1504*4]();
    
    blender = new CBlenderWrapper;
    if (blender) {
        blender->capabilityAssessment();
        blender->getSingleInstance(CBlenderWrapper::FOUR_CHANNELS);
        blender->initializeDevice();
        LOGINFO("Create blender context successfully.");
    }
    else
    {
        LOGERR("Failed to create blender context.");
        return CODEC_ERR;
    }
    
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
    
    if (blender) {
        delete blender;
        blender = nullptr;
    }
    
    if (blendedImage) {
        delete [] blendedImage;
        blendedImage = nullptr;
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

std::shared_ptr<DecodeFrame2> H264DecImpl::Decode2(unsigned char* data, unsigned int len, long long pts, long long dts, int type, std::string offset)
{
    auto frame = Decode(data, len, pts, dts);
    if (!frame)
    {
        LOGERR("Failed to decode frame...");
        return nullptr;
    }
    
    offset_ = offset;
    
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
            
            // convert to RGBA from YUV422P and blend image
            bool ret = blendImage(decode_frame, type);
            if (ret) {
                auto blendedFrame = std::make_shared<DecodeFrame2>();
                blendedFrame->pts = frame->pts;
                blendedFrame->dts = frame->dts;
                blendedFrame->private_type = DEC_PRI_DATA_TYPE_ARRY;
                blendedFrame->data = new unsigned char[params.output_width*params.output_height*2]();
                memcpy(blendedFrame->data, blendedImage, params.output_width*params.output_height*2);
                //cvtColorSpace(blendedFrame->data);
                
                blendedFrame->private_data = blendedFrame->data;
                blendedFrame->len = type;
                decode_frame = nullptr;
                
                return blendedFrame;
            }
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

void H264DecImpl::cvtColorSpace(unsigned char* data)
{
    SwsContext* rgb2yuvCxt = nullptr;
    
    uint8_t *src_data[4];
    int src_linesize[4];
    
    uint8_t *dst_data[4];
    int dst_linesize[4];
    
    int ret=0;
    ret= av_image_alloc(src_data, src_linesize, params.output_width, params.output_height, AV_PIX_FMT_RGBA, 1);
    if (ret< 0) {
        LOGERR( "Could not allocate source image\n");
        return;
    }
    ret = av_image_alloc(dst_data, dst_linesize, params.output_width, params.output_height, AV_PIX_FMT_YUV422P, 1);
    if (ret< 0) {
        LOGERR("Could not allocate destination image\n");
        av_freep(&src_data[0]);
        return;
    }
    
    rgb2yuvCxt = sws_getContext(params.output_width, params.output_height, AV_PIX_FMT_RGBA, params.output_width, params.output_height, AV_PIX_FMT_YUV422P, SWS_BICUBIC, NULL, NULL, NULL);
    if (!rgb2yuvCxt) {
        LOGERR("YUV422P to RGBA sws_getContext() failed...");
        av_freep(&src_data[0]);
        av_freep(&dst_data[0]);
        return;
    }
    
    memcpy(src_data[0], blendedImage, params.output_width*params.output_height*4);
    
    // RGBA to YUV422P
    sws_scale(rgb2yuvCxt, src_data, src_linesize, 0, params.output_height, dst_data, dst_linesize);
    
    memcpy(data, dst_data[0], params.output_width*params.output_height);
    memcpy(data + params.output_width*params.output_height, dst_data[1], params.output_width*params.output_height/2);
    memcpy(data + params.output_width*params.output_height*3/2, dst_data[2], params.output_width*params.output_height/2);
    
    sws_freeContext(rgb2yuvCxt);
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
}

bool H264DecImpl::blendImage(std::shared_ptr<DecodeFrame2> dframe, int type)
{
    SwsContext* yuv2rgbCxt;
    
    uint8_t *src_data[4];
    int src_linesize[4];
    
    uint8_t *dst_data[4];
    int dst_linesize[4];
    
    params.input_width = width_;
    params.input_height = width_;
    switch (type)
    {
            // YUV422P: 1472x736x2
        case 2166784:
        {
            params.output_width = 1472;
            params.output_height = 736;
            break;
        }
            // YUV422P: 2176x1088x2
        case 4734976:
        {
            params.output_width = 2176;
            params.output_height = 1088;
            break;
        }
            // YUV422P: 3008x1504x2
        case 9048064:
        {
            params.output_width = 3008;
            params.output_height = 1504;
            break;
        }
    }
    
    int ret=0;
    ret= av_image_alloc(src_data, src_linesize, width_, height_, AV_PIX_FMT_YUV422P, 1);
    if (ret< 0) {
        LOGERR( "Could not allocate source image\n");
        return false;
    }
    ret = av_image_alloc(dst_data, dst_linesize, params.output_width, params.output_height, AV_PIX_FMT_UYVY422, 1);
    if (ret< 0) {
        LOGERR("Could not allocate destination image\n");
        av_freep(&src_data[0]);
        return false;
    }
    
    yuv2rgbCxt = sws_getContext(width_, height_, AV_PIX_FMT_YUV422P, params.output_width, params.output_height, AV_PIX_FMT_UYVY422, SWS_BICUBIC, NULL, NULL, NULL);
    if (!yuv2rgbCxt) {
        LOGERR("YUV to RGBA sws_getContext() failed...");
        av_freep(&src_data[0]);
        av_freep(&dst_data[0]);
        return false;
    }
    
    memcpy(src_data[0], dframe->data, width_*height_);                     //Y
    memcpy(src_data[1], dframe->data+width_*height_, width_*height_/2);      //U
    memcpy(src_data[2], dframe->data+width_*height_*3/2, width_*height_/2);   //V
    
    // YUV422P to RGBA
    sws_scale(yuv2rgbCxt, src_data, src_linesize, 0, height_, dst_data, dst_linesize);
    
    memcpy(blendedImage, dst_data[0], params.output_width*params.output_height*2);
    
    LOGINFO("Input width: %d, Input height: %d, Output width: %d, Output height: %d", width_, height_, params.output_width, params.output_height);
    
    
    sws_freeContext(yuv2rgbCxt);
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    
    return true;
}

