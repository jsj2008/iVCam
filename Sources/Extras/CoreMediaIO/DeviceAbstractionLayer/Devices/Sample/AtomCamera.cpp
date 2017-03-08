//
// Created by julee on 16-11-19.
//

#include "AtomCamera.h"
#include <chrono>
#include "Error.h"
#include "Frame.h"
#include "log.h"
#include <stdlib.h>
#include <sys/types.h>

using namespace std;
using namespace chrono;

static const uint8_t UVC_EXTENSION_UNIT_CS_IO_START = 0x0E;
static const uint8_t UVC_EXTENSION_UNIT_CS_IO_READ_DATA = 0x0B;
static const uint8_t UVC_EXTENSION_UNIT_CS_IO_END = 0x0E;
static const uint8_t INDEX_INSTA_DATA_UUID = 5;
static const uint8_t EXTENSION_UNIT_ID_VENDOR = 6;

typedef unsigned char BYTE;

static bool checkStillImage(const shared_ptr<Frame> &frame)
{
    if(frame->size() < 4)
        return false;
    uint8_t *data = frame->data();
    return data[0]  == 0xFF && data[1] == 0xD8 && data[2]  == 0xFF;
}

int AtomCamera::open(StreamFormat format, int width, int height, int fps, int bitrate)
{
    LOGINFO("UVC Device found");
    int retv = ERR_UNKNOWN;
    shared_ptr<Frame> frame;
    uvc_error_t res = uvc_open(mUvcDevice, &mUvcDevicheHandle);
    if(res < 0)
    {
        LOGINFO("uvc_open error: %s", uvc_strerror(res));
        goto error;
    }

    LOGINFO("UVC Device opened");

    mStreamCtrl = (uvc_stream_ctrl_t *)malloc(sizeof(*mStreamCtrl));
    res = uvc_get_stream_ctrl_format_size(
        mUvcDevicheHandle, mStreamCtrl, /* result stored in ctrl */
        format == StreamFormat::H264 ? UVC_FRAME_FORMAT_FRAME_BASED_H264 : UVC_FRAME_FORMAT_MJPEG,
        width, height, fps /* width, height, fps */
    );
    if(res < 0)
    {
        LOGINFO("uvc device set unsupported params group:%d", res);
        goto error;
    }

    res = (uvc_error_t)uvcext_set_stream_bitrate_xu(mUvcDevicheHandle, bitrate);
    if(res < 0)
    {
        LOGINFO("failed set bitrate(%d): %d", bitrate, res);
        goto error;
    }

    res = uvc_start_streaming2(mUvcDevicheHandle, mStreamCtrl,UVC_FRAME_FORMAT_ANY,3008,1504, NULL, NULL, 0, &mStreamHandle);
    if(res < 0)
    {
        LOGINFO("start_streaming error: %s", uvc_strerror(res));
        goto error;
    }

    LOGINFO("try read one video frame");
    retv = readFrame(&frame);
    if(retv != 0)
    {
        LOGINFO("read frame failed: %d", retv);
        goto error;
    }
    LOGINFO("got one frame, size: %d(start bytes: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x)",
        frame->size(),
        (int)frame->data()[0], (int)frame->data()[1], (int)frame->data()[2], (int)frame->data()[3],
        (int)frame->data()[4], (int)frame->data()[5], (int)frame->data()[6], (int)frame->data()[7]);

    LOGINFO("pause stream");
    uvc_pause_streaming(mStreamHandle);
    
    return 0;

error:
    /* Release our handle on the device */
    if(mUvcDevicheHandle)
    {
        LOGINFO("close device handle");
        uvc_close(mUvcDevicheHandle);
        mUvcDevicheHandle = nullptr;
    }
    /* Release the device descriptor */
    if(mUvcDevice)
    {
        LOGINFO("unref device");
        uvc_unref_device(mUvcDevice);
        mUvcDevice = nullptr;
    }

    return ERR_INIT;
}

void AtomCamera::close()
{
    if(mUvcDevicheHandle != nullptr)
    {
        LOGINFO("stop streaming");
        uvc_stop_streaming(mUvcDevicheHandle);
        LOGINFO("close device handle");
        uvc_close(mUvcDevicheHandle);
        mUvcDevicheHandle = nullptr;
    }
    if(mUvcDevice != nullptr)
    {
        LOGINFO("unref device");
        uvc_unref_device(mUvcDevice);
        mUvcDevice = nullptr;
    }
    mClosed = true;
    free(mStreamCtrl);
    mStreamCtrl = nullptr;
}

AtomCamera::AtomCamera(string name, uvc_device_t *device, uvc_context_t *context) :
    mTag("Insta360Air" + name), mName(name), mUvcContext(context), mUvcDevice(device)
{
    LOGINFO("camera name: %s", name.c_str());
    TAG = mTag.c_str();
    uvc_ref_device(mUvcDevice);
}

static const int kMaxReadFrameTimeUs = 10*1000*1000;

int AtomCamera::readFrame(std::shared_ptr<Frame> *pframe)
{
    struct uvc_frame *uvcFrame = NULL;
    uvc_error_t err = uvc_stream_get_frame(mStreamHandle, &uvcFrame, kMaxReadFrameTimeUs);
    if(err != 0)
    {
        LOGINFO("Failed to read frame. Error code: %d", err);
        return ERR_IO;
    }
    
    if(uvcFrame == NULL)
        return ERR_TIMEOUT;
    *pframe = shared_ptr<Frame>(new Frame(uvcFrame->data, uvcFrame->data_bytes, uvcFrame->is_still_image));
    
    return 0;
}

std::string AtomCamera::readOffset()
{
    char* offset = nullptr;
    int len = 0;
    uvcext_read_offset_20(mUvcDevicheHandle, &offset, &len);
    if (offset) {
        mOffset = std::string(offset);
        free(offset);
    }
    
    return mOffset;
}


bool AtomCamera::isClosed()
{
    return mClosed;
}

std::string AtomCamera::readCameraUUID()
{
    char *uuid=nullptr;
    int len=0;
    uvcext_read_uuid_20(mUvcDevicheHandle, &uuid, &len);
    if(uuid)
    {
        mUUID = std::string(uuid);
        free(uuid);
    }
    return mUUID;
}


