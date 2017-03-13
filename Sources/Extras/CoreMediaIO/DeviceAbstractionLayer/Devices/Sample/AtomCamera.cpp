//
// Created by julee on 16-11-19.
//

#include "AtomCamera.h"
#include "Error.h"
#include "Frame.h"
#include "log.h"
#include <chrono>
#include <stdlib.h>
#include <sys/types.h>

using namespace std;
using namespace chrono;

typedef unsigned char BYTE;
  
int AtomCamera::open(StreamFormat format, int width, int height, int fps, int bitrate)
{
    int retv = ERR_UNKNOWN;
    shared_ptr<Frame> frame;
    if (!mInitialStatus) {
        return ERR_INIT;
    }
    uvc_error_t res = uvc_open(*mUvcDevice, &mUvcDevicheHandle);
    if(res < 0)
    {
        LOGINFO("UVC open error: %s", uvc_strerror(res));
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
        LOGINFO("UVC device set unsupported params group:%d", res);
        goto error;
    }

    res = (uvc_error_t)uvcext_set_stream_bitrate_xu(mUvcDevicheHandle, bitrate);
    if(res < 0)
    {
        LOGINFO("Failed set bitrate(%d): %d", bitrate, res);
        goto error;
    }

    res = uvc_start_streaming2(mUvcDevicheHandle, mStreamCtrl,UVC_FRAME_FORMAT_ANY,3008,1504, NULL, NULL, 0, &mStreamHandle);
    if(res < 0)
    {
        LOGINFO("Start_streaming error: %s", uvc_strerror(res));
        goto error;
    }
    
    retv = readFrame(&frame);
    if(retv != 0)
    {
        LOGINFO("Read frame failed: %d", retv);
        goto error;
    }
    LOGINFO("Got one frame, size: %d(start bytes: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x)",
        frame->size(),
        (int)frame->data()[0], (int)frame->data()[1], (int)frame->data()[2], (int)frame->data()[3],
        (int)frame->data()[4], (int)frame->data()[5], (int)frame->data()[6], (int)frame->data()[7]);
    
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
        uvc_unref_device(*mUvcDevice);
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
        uvc_unref_device(*mUvcDevice);
        mUvcDevice = nullptr;
    }
    mClosed = true;
    free(mStreamCtrl);
    mStreamCtrl = nullptr;
}

AtomCamera::AtomCamera()
{
    int numDevs;
    uvc_init(NULL ,&mUvcContext, NULL);
    int retv = uvc_get_devices(mUvcContext, &mUvcDevice, &numDevs, 0x2e1a, 0x1000);
    if(retv != 0)
    {
        mInitialStatus = false;
        if(retv == UVC_ERROR_NO_DEVICE)
        {
            LOGERR("NO UVC device found");
        }
        else
        {
            LOGINFO("Failed get uvc device list: %s", uvc_strerror((uvc_error_t) retv));
        }
    }
    else
    {
        mInitialStatus = true;
        LOGINFO("Found %d device(s)", numDevs);
        uvc_ref_device(*mUvcDevice);
    }
}

AtomCamera::AtomCamera(string name, uvc_device_t **device, uvc_context_t *context) :
    mTag("Insta360Air" + name), mName(name), mUvcContext(context), mUvcDevice(device), mInitialStatus(true)
{
    LOGINFO("Camera name: %s", name.c_str());
    TAG = mTag.c_str();
    uvc_ref_device(*mUvcDevice);
}

static const int kMaxReadFrameTimeUs = 10*1000*1000;

int AtomCamera::readFrame(std::shared_ptr<Frame> *pframe)
{
    if (!mInitialStatus) {
        return ERR_INIT;
    }
    struct uvc_frame *uvcFrame = NULL;
    uvc_error_t err = uvc_stream_get_frame(mStreamHandle, &uvcFrame, kMaxReadFrameTimeUs);
    if(err != 0)
    {
        LOGINFO("Failed to read frame. Error code: %d", err);
        return ERR_IO;
    }
    
    if(uvcFrame == NULL)
    {
        LOGINFO("Time out to fetch UVC frame!");
        return ERR_TIMEOUT;
    }
    
    *pframe = shared_ptr<Frame>(new Frame(uvcFrame->data, uvcFrame->data_bytes));
    
    return 0;
}

bool AtomCamera::isClosed()
{
    return mClosed;
}

int AtomCamera::getCameraUUID(std::string& UUID)
{
    if (!mInitialStatus)
    {
        return ERR_INIT;
    }
    
    char* temp = nullptr;
    int len = 0;
    // version 2.0 of function 'uvcext_read_uuid()'
    uvcext_read_uuid_20(mUvcDevicheHandle, &temp, &len);
    if (temp) {
        UUID = std::string(temp);
        free(temp);
    }
    else
    {
        LOGINFO("Failed to get UUID from device.");
        return ERR_UNKNOWN;
    }
    
    return 0;
}

int AtomCamera::getCameraOffset(std::string& offset)
{
    if (!mInitialStatus)
    {
        return ERR_INIT;
    }
    
    char* temp = nullptr;
    int len = 0;
    // version 2.0 of function 'uvcext_read_offset()'
    uvcext_read_offset_20(mUvcDevicheHandle, &temp, &len);
    if (temp) {
        offset = std::string(temp);
        free(temp);
    }
    else
    {
        LOGINFO("Failed to get offset from device.");
        return ERR_UNKNOWN;
    }
    
    return 0;
}
