/*
	    File: IOVideoSampleDevice.cpp
	Abstract: n/a
	 Version: 1.2
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
	Inc. ("Apple") in consideration of your agreement to the following
	terms, and your use, installation, modification or redistribution of
	this Apple software constitutes acceptance of these terms.  If you do
	not agree with these terms, please do not use, install, modify or
	redistribute this Apple software.
	
	In consideration of your agreement to abide by the following terms, and
	subject to these terms, Apple grants you a personal, non-exclusive
	license, under Apple's copyrights in this original Apple software (the
	"Apple Software"), to use, reproduce, modify and redistribute the Apple
	Software, with or without modifications, in source and/or binary forms;
	provided that if you redistribute the Apple Software in its entirety and
	without modifications, you must retain this notice and the following
	text and disclaimers in all such redistributions of the Apple Software.
	Neither the name, trademarks, service marks or logos of Apple Inc. may
	be used to endorse or promote products derived from the Apple Software
	without specific prior written permission from Apple.  Except as
	expressly stated in this notice, no other rights or licenses, express or
	implied, are granted by Apple herein, including but not limited to any
	patent rights that may be infringed by your derivative works or by other
	works in which the Apple Software may be incorporated.
	
	The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
	MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
	THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
	OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
	
	IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
	MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
	AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
	STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
	
	Copyright (C) 2012 Apple Inc. All Rights Reserved.
	
*/

#include "IOVideoDebugMacros.h"

// Self Include
#include "IOVideoSampleDevice.h"

// Internal Includes
#include "IOVideoSampleStream.h"
#include "IOVideoSampleDeviceShared.h"
#include "CMIO_KEXT_Sample_ControlIDs.h"

// System Includes
#include <IOKit/usb/IOUSBLog.h>
#include <IOKit/video/IOVideoStreamDictionary.h>
#include <IOKit/video/IOVideoStreamFormatDictionary.h>
#include <IOKit/video/IOVideoControlDictionary.h>

namespace
{
	enum
	{
		kInputStreamID	= 1,
		kOutputStreamID = 2
	};

}

#define kMaxOutputBuffers 10

#define RELEASE_IF_SET(x) do { if (x) { (x)->release(); (x) = 0; } } while (0)

#define kTimerIntervalNTSC 33366
#define kTimerInterval30 33333
#define kTimerInterval24 41666
#define kTimerInterval2397 41708
#define kTimerIntervalPAL 40000
#define kTimerIntervalHD50 20000
#define kTimerIntervalHD5994 16683
#define kTimerIntervalHD60 16666

// DV frames are 120, 000 bytes long
#define kYUVVFrameSize (691200)
#define kYUVDataSize (kYUVVFrameSize)

#define kFullNTSCYUVVFrameSize (699840)
#define kFullNTSCYUVDataSize (kFullNTSCYUVVFrameSize)

#define kPALYUVVFrameSize (829440)
#define kPALYUVDataSize (kFullNTSCYUVVFrameSize)

#define kHD720pYUVVFrameSize (1843200)
#define kHD720pYUVDataSize (kHD720pYUVVFrameSize)

#define kHD1080pYUVVFrameSize (4147200)
#define kHD1080pYUVDataSize (kHD1080pYUVVFrameSize)

#define kYUVV_10_FrameSize (921600)
#define kYUV_10_DataSize (kYUVV_10_FrameSize)

#define kFullNTSCYUVV_10_FrameSize (933120)
#define kFullNTSCYUV_10_DataSize (kFullNTSCYUVV_10_FrameSize)

#define kPALYUVV_10_FrameSize (1105920)
#define kPALYUV_10_DataSize (kPALYUVV_10_FrameSize)

#define kHD720pYUVV_10_FrameSize (2489760)
#define kHD720pYUV_10_DataSize (kHD720pYUVV_10_FrameSize)

#define kHD1080pYUVV_10_FrameSize (5529600)
#define kHD1080pYUV_10_DataSize (kHD1080pYUVV_10_FrameSize)


#define MAX_FRAME_SIZE							(1080*1920*4)

#if DEBUG
#define DEBUG_LOG(format, ...) IOLog("%s[%d]: " format, __PRETTY_FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_LOG(format, ...)
#endif

#define super IOVideoDevice

OSDefineMetaClassAndStructors(IOVideoSampleDevice, IOVideoDevice)


void IOVideoSampleDevice::timerFired(OSObject* owner, IOTimerEventSource* timer)
{
	IOVideoSampleDevice* device;
	DEBUG_LOG("timer fired\n");
	
	device = OSDynamicCast(IOVideoSampleDevice, owner);
	if (device)
	{
		if (device->mCurrentDirection)
		{
#if START_SENDING_ON_TWO_SECOND_BOUNDRY
			if (device->mWaitingToStart)
			{
				uint64_t now;
				clock_get_uptime(&now);
				device->mWaitingToStart = (now < device->mFirstInputAtTwoSecondBoundaryHostTimeUptime);
			}
			if ( !(device->mWaitingToStart) )
				device->sendOutputFrame();
#else
            DEBUG_LOG("sendOutputFrame");
			device->sendOutputFrame();
#endif
		}
		else
		{
            DEBUG_LOG("consumeInputFrame");
			device->consumeInputFrame();
		}
		
		timer->setTimeoutUS(device->mTimeInterval);
	} else
	{
		DEBUG_LOG("help, no device!\n");
	}
}


extern "C" {
	
#define MACH_KERNEL 1
#include <mach-o/loader.h>
#if __LP64__
extern void *getsectdatafromheader(struct mach_header_t*, char*, char*, size_t*);
#else
extern void *getsectdatafromheader(struct mach_header_t*, char*, char*, size_t*);
#endif
	
}

void IOVideoSampleDevice::free()
{
    DEBUG_LOG("IOVideoSampleDevice::free");
	if (_timer) _timer->cancelTimeout();

	RELEASE_IF_SET(_timer);
	RELEASE_IF_SET(_workloop);

	super::free();
}

bool IOVideoSampleDevice::start(IOService* provider)
{
    DEBUG_LOG("IOVideoSampleDevice::start");
	KernelDebugEnable(true);
	KernelDebugSetLevel(7);
	KernelDebugSetOutputType(kKernelDebugOutputKextLoggerType);
	DEBUG_LOG("This is a test\n");

	if (! super::start(provider))
		return false;
	
	DEBUG_LOG("start\n");
	
	_outputStream = NULL;
	_inputStream = NULL;
	mFrameCountFromStart = 0;
	mDroppedFrameCount = 0;
    mLastDisplayedSequenceNumber = 0;
    mTimeInterval = kTimerIntervalNTSC;
	
	mCurrentDirection = true;
	do
	{
		OSArray*		theControlList = NULL;
		OSDictionary*	theControl = NULL;
		IOReturn result;
		OSArray*		theSourceSelectorMap = NULL;
		OSDictionary*	theSelectorControl =  NULL;
		OSDictionary*	theSelectorItem = NULL;

//		struct mach_header_t*  mh;
//		extern kmod_info_t kmod_info;

//		mh = (struct mach_header_t*)kmod_info.address;
//		mYUVData = (char*)getsectdatafromheader(mh,
//										(char*) "YUV_DATA",
//										(char*)	 "yuv_data",
//										&mYUVSize);
			  
//		if (NULL == mYUVData)
//		{
//			DEBUG_LOG("couldn't get YUV_DATA section\n");
//			break;
//		}
//
//		mNumBuffers = mYUVSize / kYUVVFrameSize;
//		mh = (struct mach_header_t*)kmod_info.address;
//		mHD720pYUVData = (char*)getsectdatafromheader(mh,
//												  (char*) "HD720YUV_DATA",
//												  (char*) "hd720yuv_data",
//												 &mHD720pYUVSize);
//
//		if (NULL == mHD720pYUVData)
//		{
//			DEBUG_LOG("couldn't get HD720YUV_DATA section\n");
//			break;
//		}
//
//		mh = (struct mach_header_t*)kmod_info.address;
//		mHD1080pYUVData = (char*)getsectdatafromheader(mh,
//													   (char*) "HD1080YUV_DATA",
//														(char*) "hd1080yuv_data",
//													   &mHD1080pYUVSize);
//
//		if (NULL == mHD1080pYUVData)
//		{
//			DEBUG_LOG("couldn't get HD1080YUV_DATA section\n");
//			break;
//		}


//		mNumBuffers = mYUVSize / kYUVVFrameSize;
        mNumBuffers = 30;
		mMaxNumBuffers = mNumBuffers;
 
		result = AllocateFrameBuffers(MAX_FRAME_SIZE*mNumBuffers);
		if (result != kIOReturnSuccess)
		{
			DEBUG_LOG("Starting IOVideoSampleDevice - AllocateFrameBuffers failed\n");
			break;
		}

		result = AllocateControlBuffers(sizeof(struct SampleVideoDeviceControlBuffer)*mNumBuffers);
		if (result != kIOReturnSuccess)
		{
			DEBUG_LOG("Starting IOVideoSampleDevice - AllocateFrameBuffers failed\n");
			break;
		}
        
        ZeroControlBuffer(mControlBuf,sizeof(struct SampleVideoDeviceControlBuffer)*mNumBuffers);
		 
		// write colors to the frame buffer for content
//		int i;
//		for (i=0; i<mNumBuffers; i++)
//		{
//			LoadFrameBuffer(mFrameBuf+(i*MAX_FRAME_SIZE), mYUVData, kYUVVFrameSize, i);
//		}

		DEBUG_LOG("IOVideoSampleDevice::start : got yuv_data of size %x\n", (unsigned int)  mYUVSize);

		//	create the array that will hold the controls
		theControlList = OSArray::withCapacity(1);
		if (!theControlList)
		{
			DEBUG_LOG("IOVideoSampleDevice::start : couldn't allocate the control list\n");
			break;
		}
		//	create a custom play through boolean control
		theControl = IOVideoControlDictionary::createBooleanControl(CMIO::KEXT::Sample::kDirectionControlID, kIOVideoControlBaseClassIDBoolean, kIOVideoBooleanControlClassIDDirection, kIOVideoControlScopeGlobal, kIOVideoControlElementMaster, mCurrentDirection, false, 0, OSString::withCString("Direction Control"));
		if (!theControl)
		{
			DEBUG_LOG("IOVideoSampleDevice::start: couldn't allocate the direction control\n");
			break;
		}
		
		//	put the custom play through boolean control into the list
		theControlList->setObject(theControl);
		
		theSourceSelectorMap = OSArray::withCapacity(3);
		if (!theSourceSelectorMap)
		{
			DEBUG_LOG("IOVideoSampleDevice::start: couldn't allocate the source selector map array\n");
			break;
		}
		theSelectorItem = IOVideoControlDictionary::createSelectorControlSelectorMapItem((UInt32)1, OSString::withCString("Tuner"));
		theSourceSelectorMap->setObject(theSelectorItem);
		theSelectorItem->release();
		theSelectorItem = IOVideoControlDictionary::createSelectorControlSelectorMapItem((UInt32)2, OSString::withCString("Composite"));
		theSourceSelectorMap->setObject(theSelectorItem);
		theSelectorItem->release();
		theSelectorItem = IOVideoControlDictionary::createSelectorControlSelectorMapItem((UInt32)3, OSString::withCString("S-Video"));
		theSourceSelectorMap->setObject(theSelectorItem);
		theSelectorItem->release();
		
		//	create a data source control
		theSelectorControl = IOVideoControlDictionary::createSelectorControl(CMIO::KEXT::Sample::kInputSourceSelectorControlID, kIOVideoControlBaseClassIDSelector, kIOVideoSelectorControlClassIDDataSource, kIOVideoControlScopeGlobal, kIOVideoControlElementMaster, CMIO::KEXT::Sample::kSampleSourceSVideo, theSourceSelectorMap, false, 0, OSString::withCString("Video Source"));
		if (!theSelectorControl)
		{
			DEBUG_LOG("IOVideoSampleDevice::start: couldn't allocate the theSelectorControl control\n");
			break;
		}
		
		theControlList->setObject(theSelectorControl);
				
		
		//	put the control list into the registry
		setProperty(kIOVideoDeviceKey_ControlList, theControlList);
	
	
		if (mCurrentDirection)
		{
			if (!AddInputStreams())
				break;
		}
		else
		{
			if (!AddOutputStreams())
				break;
		}
	
	
		if (NULL != theControlList)
		{
			theControlList->release();
		}
		
		if (NULL != theControl)
		{
			theControl->release();
		}

		 
		_workloop = IOWorkLoop::workLoop();
		if (!_workloop) break;
		
		_timer = IOTimerEventSource::timerEventSource(this, timerFired);
		if (!_timer) break;
		
		_workloop->addEventSource(_timer);
				
		registerService();
		
		DEBUG_LOG("IOVideoSampleDevice start successful\n");
		
		return true;
		
	} while (false);
	
	
	if (_timer) _timer->cancelTimeout();
	
	RELEASE_IF_SET (_timer);
	RELEASE_IF_SET (_workloop);
	
	DEBUG_LOG("IOVideoSampleDevice start failed\n");
	
	return false;
}
//-------------------------------------------------------------------------------------------------------
//	stop
//-------------------------------------------------------------------------------------------------------
void IOVideoSampleDevice::stop(IOService* provider)
{
	DEBUG_LOG("Stopping IOVideoSampleDevice\n");
	
	super::stop(provider);
	
	if (mMemDescFrameBuf)							// release descriptor to data memory
	{
		mMemDescFrameBuf->release();
		mMemDescFrameBuf = NULL;
	}

	if (mMemDescControlBuf)						// release descriptor to control memory
	{
		mMemDescControlBuf->release();
		mMemDescControlBuf = NULL;
	}

	if (mCurrentDirection)
	{
		RemoveInputStreams();
	}
	else
	{
		RemoveOutputStreams();
	}

}


IOReturn IOVideoSampleDevice::sendOutputFrame(void)
{
	IOStreamBuffer* buffer;
	IOReturn result;
	UInt64	ts;
	IOMemoryDescriptor	*ctrlDescriptor;
	
	clock_get_uptime(&ts);
	
    DEBUG_LOG("sendOutputFrame %d\n", _currentBuffer);
	
	if (!_outputStream)
	{
		DEBUG_LOG("help, no output stream!\n");
		return kIOReturnNotOpen;
	}
		
	buffer = OSDynamicCast(IOStreamBuffer, _outputStream->getBufferWithID(_currentBuffer));
	if (!buffer)
	{
		DEBUG_LOG("help, no buffer at index %d!\n", (int)_currentBuffer);
		return kIOReturnNotReady;
	}
	
	ctrlDescriptor = OSDynamicCast(IOMemoryDescriptor, buffer->getControlBuffer());
	if (NULL != ctrlDescriptor)
	{
#if START_SENDING_ON_TWO_SECOND_BOUNDRY
		//	if this is the first frame output from the device, calculate the fudge factor
		//	so that our sequence started on exactly the hosttime we were shooting for;
		//	we need this because we are run by a timer that doesn't necessarily start
		//	when we want it to
		if (mCalculateHostTimeFudge)
		{
			if (ts >= mFirstInputAtTwoSecondBoundaryHostTimeUptime)
				mHostTimeFudge = ts - mFirstInputAtTwoSecondBoundaryHostTimeUptime;
			else
				mHostTimeFudge = 0LL;
			mCalculateHostTimeFudge = false;
		}
		
		//	adjust measured hosttime by our fudge factor
		ts -= mHostTimeFudge;
#endif
		
		//	build control information to return
		SampleVideoDeviceControlBuffer theBuffControl;
		theBuffControl.vbiTime = ts; 
		theBuffControl.outputTime = ts; 
		theBuffControl.totalFrameCount = mFrameCountFromStart; 
		theBuffControl.droppedFrameCount = mDroppedFrameCount; 
		theBuffControl.firstVBITime = mFirstInputAtTwoSecondBoundaryHostTimeUptime; 
		IOByteCount writeCount;
		writeCount = ctrlDescriptor->writeBytes(0, &theBuffControl, sizeof(SampleVideoDeviceControlBuffer));
	}
	else
	{
		DEBUG_LOG("NULL == ctrlDescriptor\n");
	}
	
	result = _outputStream->enqueueOutputBuffer(buffer, 0, buffer->getDataBuffer()->getLength(), 0, buffer->getControlBuffer()->getLength());
	if (result != kIOReturnSuccess)
	{
		// Probably the queue is full
		DEBUG_LOG("help, enqueueing buffer failed (%x)!\n", result);
		return result;
	}
	
	result = _outputStream->sendOutputNotification();
	if (result != kIOReturnSuccess)
	{
		DEBUG_LOG("help, sending output notification failed! (%x)!\n", result);
		return result;
	}
	
	_currentBuffer++;
	// 当前帧到末尾，重置为0
    if (_currentBuffer >= _outputStream->getBufferCount())
	{
		DEBUG_LOG("wrapping frame buffer from %d to 0\n", _currentBuffer);
		_currentBuffer = 0;
	}
	
	return kIOReturnSuccess;
}


IOReturn IOVideoSampleDevice::consumeInputFrame(void)
{
	DEBUG_LOG("consumeInputFrame\n");

	if (!_inputStream)
	{
		DEBUG_LOG("help, no input stream!\n");
		return kIOReturnNotOpen;
	}

	UInt64 ts;
    UInt64 lastDisplayedSequenceNumber = 0;
    UInt32 frameDiscontinuityFlags = 0;
    IOAudioSMPTETime   frameSMPTETime;
    
    frameSMPTETime.fFlags = 0;
    
	clock_get_uptime(&ts);
	
	
	IOStreamBuffer* buffer = _inputStream->getFilledBuffer();
	if (!buffer)
	{
		DEBUG_LOG("no filled buffers available!\n");
		mDroppedFrameCount++;
        mLastDisplayedSequenceNumber = lastDisplayedSequenceNumber;
		_inputStream->postFreeInputBuffer(ts, 0, mFrameCountFromStart, mDroppedFrameCount,mLastDisplayedSequenceNumber);
		mFrameCountFromStart++;
		return kIOReturnNotReady;
	}

	// if we get a filled buffer it can be sent to the DMA
	IOMemoryDescriptor	*ctrlDescriptor;
	
	// at this point we are done with the buffer, so we can put it back into the free
	// pool so that it can be filled with new data
    ctrlDescriptor = OSDynamicCast(IOMemoryDescriptor, buffer->getControlBuffer());
	if (NULL != ctrlDescriptor)
	{
        IOByteCount readBytes = ctrlDescriptor->readBytes(offsetof(SampleVideoDeviceControlBuffer,sequenceNumber),&lastDisplayedSequenceNumber,sizeof(UInt64));
        if (readBytes == sizeof(UInt64))
        {
            DEBUG_LOG("mLastDisplayedSequenceNumber = %lld\n",lastDisplayedSequenceNumber);

        }
        else
        {
            lastDisplayedSequenceNumber = 0;
        }
        readBytes = ctrlDescriptor->readBytes(offsetof(SampleVideoDeviceControlBuffer,discontinuityFlags),&frameDiscontinuityFlags,sizeof(UInt32));
        if (readBytes == sizeof(UInt32))
        {
            //If this flag is non-zero then the buffer was from a scrub operation
            DEBUG_LOG("discontinuityFlags = %lld\n", (long long int)frameDiscontinuityFlags);
        }
        readBytes = ctrlDescriptor->readBytes(offsetof(SampleVideoDeviceControlBuffer,smpteTime),&frameSMPTETime,sizeof(IOAudioSMPTETime));
        if (readBytes == sizeof(IOAudioSMPTETime))
        {
            if(frameSMPTETime.fFlags & kIOAudioSMPTETimeValid)
            {
                //this is a valid SMPTE time use the fType value to determine the format
                //the current IOAudioTypes.h doesn't define all the same types as CoreAudioTypes.h
                //but eventually it should
                DEBUG_LOG("smpteTime.fFlags = %ld", (long int)frameSMPTETime.fFlags);
                DEBUG_LOG("smpteTime.fCounter = %ld", (long int)frameSMPTETime.fCounter);
                DEBUG_LOG("smpteTime.fHours = %d", frameSMPTETime.fHours);
                DEBUG_LOG("smpteTime.fMinutes = %d", frameSMPTETime.fMinutes);
                DEBUG_LOG("smpteTime.fSeconds = %d", frameSMPTETime.fSeconds);
                DEBUG_LOG("smpteTime.fFrames = %d", frameSMPTETime.fFrames);
                DEBUG_LOG("smpteTime.fType = %ld",(long int)frameSMPTETime.fType);
            }
        }
    }
    
    mLastDisplayedSequenceNumber = lastDisplayedSequenceNumber;
	_inputStream->returnBufferToFreeQueue(buffer);
	_inputStream->postFreeInputBuffer(ts, ts, mFrameCountFromStart, mDroppedFrameCount,mLastDisplayedSequenceNumber);
	mFrameCountFromStart++;

	return kIOReturnSuccess;
}


IOReturn IOVideoSampleDevice::startStream(IOVideoStream* stream)
{
	DEBUG_LOG("startStream");
	mFrameCountFromStart = 0;
	mDroppedFrameCount = 0;
    mLastDisplayedSequenceNumber = 0;
		
#if START_SENDING_ON_TWO_SECOND_BOUNDRY
	if (mCurrentDirection)
	{
		uint64_t now;
		clock_get_uptime(&now);
		uint64_t nowNS;
		absolutetime_to_nanoseconds(now, &nowNS);
		uint64_t nowSec = nowNS / 1000000000ULL;
		uint64_t nowTenths = (nowNS - (nowSec * 1000000000ULL)) / 100000000ULL;
		
		if (0 == (nowSec % 2))
		{
			//	currently an even second... don't start until next even second
			nanoseconds_to_absolutetime(((nowSec + 2) * 1000000000ULL), &mFirstInputAtTwoSecondBoundaryHostTimeUptime);
		}
		else
		{
			//	currently an odd second... if we are not with .2 of the next even
			//	second, we can start on the next second.  Otherwise, wait until
			//	the follow-on even second
			if (8 >= nowTenths)
				nanoseconds_to_absolutetime(((nowSec + 1) * 1000000000ULL), &mFirstInputAtTwoSecondBoundaryHostTimeUptime);
			else
				nanoseconds_to_absolutetime(((nowSec + 3) * 1000000000ULL), &mFirstInputAtTwoSecondBoundaryHostTimeUptime);
		}
		mWaitingToStart = true;
		mCalculateHostTimeFudge = true;
		
		_currentBuffer = 0;		//	always start with the first frame
	}
#endif
    int i;

    if(!mCurrentDirection)
    {
        //for output post up some free buffers
        for (i = 0; i < 10; i++)
        {
            _inputStream->postFreeInputBuffer(0, 0, 0, 0,0);
        }
    }
  
	if (_timer) _timer->setTimeoutUS(mTimeInterval);

	return kIOReturnSuccess;
}


IOReturn IOVideoSampleDevice::stopStream(IOVideoStream* stream)
{
	DEBUG_LOG("stopStream");
	
	if (_timer) _timer->cancelTimeout();

	return kIOReturnSuccess;
}


IOReturn IOVideoSampleDevice::suspendStream(IOVideoStream* stream)
{
	DEBUG_LOG("suspendStream");
	
	if (_timer) _timer->cancelTimeout();
	
	return kIOReturnSuccess;
}


IOReturn IOVideoSampleDevice::setControlValue(UInt32 controlID, UInt32 value, UInt32* newValue)
{
	DEBUG_LOG("IOVideoSampleDevice::setControlValue(%lu, %lu)", (long unsigned int)controlID, (long unsigned int)value);
	
	IOReturn theAnswer = kIOReturnNotFound;
	
	switch(controlID)
	{
		
		case CMIO::KEXT::Sample::kDirectionControlID:
			{
				//	get the control list from the registry
				OSArray* theControlList = OSDynamicCast(OSArray, getProperty(kIOVideoDeviceKey_ControlList));
				if (NULL != theControlList)
				{
					//We need to make a copy of IORegistry property before we modify it otherwise it will panic in a 64bit kernel
					OSArray* theLocalControlList = (OSArray*)theControlList->copyCollection();
					if (NULL != theLocalControlList)
					{
						//	get the control dictionary
						OSDictionary* theControlDictionary = IOVideoControlDictionary::getControlByID(theLocalControlList, controlID);
						if (NULL != theControlDictionary)
						{
							bool currentDirection, newDirection;
							
							currentDirection = IOVideoControlDictionary::getBooleanControlValue(theControlDictionary);
							newDirection = (0 != value);
							
							if (currentDirection != newDirection)
							{
								DEBUG_LOG("IOVideoSampleDevice::setControlValue kDirectionControlID changing direction = %d", newDirection);
								if (newDirection)
								{
									RemoveOutputStreams();
									AddInputStreams();
									mCurrentDirection = true;
								}
								else
								{
									RemoveInputStreams();
									AddOutputStreams();
									mCurrentDirection = false;
								}
								//	set the new value in the registry
								IOVideoControlDictionary::setBooleanControlValue(theControlDictionary, 0 != value);
								setProperty(kIOVideoDeviceKey_ControlList,theLocalControlList);
								
								//	set the new value as the return value
								if (NULL != newValue)
								{
									*newValue = value;
								}
								
								//	send the notification that the control value changed
								sendSingleNotification(kIOVideoDeviceNotificationID_ControlValueChanged, CMIO::KEXT::Sample::kDirectionControlID, value, 0, 0, 0);
							}
							else {								
								//	set the new value as the return value
								if (NULL != newValue)
								{
									*newValue = value;
								}
							}
							//	return successfully
							theAnswer = kIOReturnSuccess;
						}
						theLocalControlList->release();
					}
				}
			}
			break;
		case CMIO::KEXT::Sample::kInputSourceSelectorControlID:
		{
			//	get the control list from the registry
			OSArray* theControlList = OSDynamicCast(OSArray, getProperty(kIOVideoDeviceKey_ControlList));
			if (NULL != theControlList)
			{
				//We need to make a copy of IORegistry property before we modify it otherwise it will panic in a 64bit kernel
				OSArray* theLocalControlList = (OSArray*)theControlList->copyCollection();
				if (NULL != theLocalControlList)
				{
					//	get the control dictionary
					OSDictionary* theControlDictionary = IOVideoControlDictionary::getControlByID(theLocalControlList, controlID);
					if (NULL != theControlDictionary)
					{
							//	set the new value in the registry
							IOVideoControlDictionary::setSelectorControlValue(theControlDictionary, value);
							setProperty(kIOVideoDeviceKey_ControlList, theLocalControlList);
							
							//	set the new value as the return value
							if (NULL != newValue)
							{
								*newValue = value;
								// Need to bang the hardware when setting CMIO::KEXT::Sample::kInputSourceSelectorControlID
							}
							
							//	send the notification that the control value changed
							sendSingleNotification(kIOVideoDeviceNotificationID_ControlValueChanged, CMIO::KEXT::Sample::kInputSourceSelectorControlID, value, 0, 0, 0);
							
							
							//	return successfully
							theAnswer = kIOReturnSuccess;
					}
					theLocalControlList->release();
				}
			}
		}
			break;
	};
	
	return theAnswer;
}


IOReturn IOVideoSampleDevice::setStreamFormat(UInt32 streamID, const IOVideoStreamDescription* newStreamFormat)
{
	DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() streamID = %lu format = %lu codecFlags = %x", (long unsigned int)streamID, (long unsigned int)newStreamFormat->mVideoCodecType,(unsigned int)newStreamFormat->mVideoCodecFlags);
	//	first, we need to be sure we were handed properly sized data and a valid stream ID
	IOReturn theAnswer = kIOReturnBadArgument;
	if (((streamID == kInputStreamID) || (streamID == kOutputStreamID))	 && (NULL != newStreamFormat))
	{
		//	we know we have a valid format, but life is easier if we fill out the sample rate wild card now if it's present
		IOVideoStreamDescription theNewFormat = *newStreamFormat;
		if (mCurrentDirection)
		{
			//	get the output stream list
			OSArray* theInputStreamList = OSDynamicCast(OSArray, getProperty(kIOVideoDeviceKey_InputStreamList));
			if (NULL != theInputStreamList)
			{
				//We need to make a copy of IORegistry property before we modify it otherwise it will panic in a 64bit kernel
				OSArray* theLocalInputStreamList = (OSArray*)theInputStreamList->copyCollection();
				if (NULL != theLocalInputStreamList)
				{
				
					DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got input list");
					//	get the stream dictionary (this driver only has a single input stream and we already validated the ID as good)
					OSDictionary* theStreamDictionary = OSDynamicCast(OSDictionary, theLocalInputStreamList->getObject(0));
					if (NULL != theStreamDictionary)
					{
						//	make sure that the sample format is supported
						DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got stream dictionary");
						OSArray* theSupportedFormats = IOVideoStreamDictionary::copyAvailableFormats(theStreamDictionary);
						if (NULL != theSupportedFormats)
						{
							//	iterate through the supported formats and see if the new one matches any of them
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() supported format");
							IOVideoStreamDescription theSanitizedNewFormat;
							UInt32 theFormatIndex = 0xFFFFFFFF;
							UInt32 theNumberSupportedFormats = theSupportedFormats->getCount();
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() supported format count = %lu", (long unsigned int)theNumberSupportedFormats);
							for(UInt32 theSupportedFormatIndex = 0; (0xFFFFFFFF == theFormatIndex) && (theSupportedFormatIndex < theNumberSupportedFormats); ++theSupportedFormatIndex)
							{
								//	get the format dictionary
								OSDictionary* theFormatDictionary = OSDynamicCast(OSDictionary, theSupportedFormats->getObject(theSupportedFormatIndex));
								if (NULL != theFormatDictionary)
								{							
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got format dictionary");
									IOVideoStreamDescription theSupportedFormat;
									IOVideoStreamFormatDictionary::getDescription(theFormatDictionary, theSupportedFormat);
									//	check to see if it was supported
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() format = %lu", (long unsigned int)theSupportedFormat.mVideoCodecType);
									if (IOVideoStreamFormatDictionary::isSameSampleFormat(theSupportedFormat, theNewFormat))
									{
										DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() found matching format %lu", (long unsigned int)theSupportedFormatIndex);
										//	it is, so fill out the new format struct
										theSanitizedNewFormat = theSupportedFormat;
										
										//	mark that we found it
										theFormatIndex = theSupportedFormatIndex;
									}
								}
							}
							
							//	release the supported formats array
							theSupportedFormats->release();
							
							//	if the format is supported, we need to make a config change request in order to do it
							if (0xFFFFFFFF != theFormatIndex)
							{
								//	but only if the new format is different from the current format
								DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() theFormatIndex is %lu", (long unsigned int)theFormatIndex);
								DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() sanitized codecFlags = %x currentCodeFlags is %x", (unsigned int)theSanitizedNewFormat.mVideoCodecFlags,(unsigned int)mCurrentInputStreamFormat.mVideoCodecFlags);
                                
								if (!IOVideoStreamFormatDictionary::isSameSampleFormat(theSanitizedNewFormat, mCurrentInputStreamFormat))
								{
									//	This driver is painfully simple, so we will encode the new format into the
									//	change request arguments. We use the first 32 bit agument to signify that
									//	we're doing a stream format change using the user client method ID. The
									//	second 32 bit argument will hold the stream ID of stream that is changing
									//	format. The first 64 bit argument holds the new sample rate. The second 64
									//	bit argument holds the index into the array of supported formats that
									//	represents the new sample format.
									//requestConfigChange(kIOAudio2UserClientMethodID_ChangeStreamFormat, kOutputStreamID, theSanitizedNewFormat.mSampleRate, theFormatIndex);
									//change the format.
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() change the format");
									// twiddle the hardware here to change format
									
									mCurrentInputStreamFormat = theSanitizedNewFormat;
									
									IOVideoStreamDictionary::setCurrentFormat(theStreamDictionary, mCurrentInputStreamFormat);
									
									setProperty(kIOVideoDeviceKey_InputStreamList,theLocalInputStreamList);
									
									
									switch (mCurrentInputStreamFormat.mVideoCodecType)
									{
										case kYUV422_720x480:
										{
                                            mTimeInterval = kTimerIntervalNTSC;
										}
											break;
										case kYUV422_1280x720:
										{
                                            mTimeInterval = kTimerIntervalNTSC;
										}
											break;
										case kYUV422_1920x1080:
										{
                                            mTimeInterval = kTimerIntervalNTSC;
										}
											break;
									}
									ResetInputStreams();
								}
								else
								{
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() formats are not different");
								}
								
								//	make sure we return that we were successful
								theAnswer = kIOReturnSuccess;
							}
						}
						else
						{
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() no supported formats");
							//	when there are no explicitly supported formats, the stream's format can't be changed
							//	but we should return a bad format error if the new format is different from the current format
							if (IOVideoStreamFormatDictionary::isSameSampleFormat(theNewFormat, mCurrentInputStreamFormat))
							{
								theAnswer = kIOReturnSuccess;
							}
						}
					}
					theLocalInputStreamList->release();
				}
			}
		}
		else
		{
			//	get the output stream list
			OSArray* theOutputStreamList = OSDynamicCast(OSArray, getProperty(kIOVideoDeviceKey_OutputStreamList));
			if (NULL != theOutputStreamList)
			{
				//We need to make a copy of IORegistry property before we modify it otherwise it will panic in a 64bit kernel
				OSArray* theLocalOutputStreamList = (OSArray*)theOutputStreamList->copyCollection();
				if (NULL != theLocalOutputStreamList)
				{
					
					DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got output list");
					//	get the stream dictionary (this driver only has a single input stream and we already validated the ID as good)
					OSDictionary* theStreamDictionary = OSDynamicCast(OSDictionary, theLocalOutputStreamList->getObject(0));
					if (NULL != theStreamDictionary)
					{
							//	make sure that the sample format is supported
						DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got stream dictionary");
						OSArray* theSupportedFormats = IOVideoStreamDictionary::copyAvailableFormats(theStreamDictionary);
						if (NULL != theSupportedFormats)
						{
							//	iterate through the supported formats and see if the new one matches any of them
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() supported format");
							IOVideoStreamDescription theSanitizedNewFormat;
							UInt32 theFormatIndex = 0xFFFFFFFF;
							UInt32 theNumberSupportedFormats = theSupportedFormats->getCount();
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() supported format count = %lu", (long unsigned int)theNumberSupportedFormats);
							for(UInt32 theSupportedFormatIndex = 0; (0xFFFFFFFF == theFormatIndex) && (theSupportedFormatIndex < theNumberSupportedFormats); ++theSupportedFormatIndex)
							{
								//	get the format dictionary
								OSDictionary* theFormatDictionary = OSDynamicCast(OSDictionary, theSupportedFormats->getObject(theSupportedFormatIndex));
								if (NULL != theFormatDictionary)
								{							
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() got format dictionary");
									IOVideoStreamDescription theSupportedFormat;
									IOVideoStreamFormatDictionary::getDescription(theFormatDictionary, theSupportedFormat);
									//	check to see if it was supported
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() format = %lu", (long unsigned int)theSupportedFormat.mVideoCodecType);
									if (IOVideoStreamFormatDictionary::isSameSampleFormat(theSupportedFormat, theNewFormat))
									{
										DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() found matching format %lu", (long unsigned int)theSupportedFormatIndex);
										//	it is, so fill out the new format struct
										theSanitizedNewFormat = theSupportedFormat;
										
										//	mark that we found it
										theFormatIndex = theSupportedFormatIndex;
									}
								}
							}
							
							//	release the supported formats array
							theSupportedFormats->release();
							
							//	if the format is supported, we need to make a config change request in order to do it
							if (0xFFFFFFFF != theFormatIndex)
							{
								//	but only if the new format is different from the current format
								DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() theFormatIndex is %lu", (long unsigned int)theFormatIndex);
								DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() sanitized codecFlags = %x currentCodeFlags is %x", (unsigned int)theSanitizedNewFormat.mVideoCodecFlags,(unsigned int)mCurrentOutputStreamFormat.mVideoCodecFlags);
								if (!IOVideoStreamFormatDictionary::isSameSampleFormat(theSanitizedNewFormat, mCurrentOutputStreamFormat))
								{
									//	This driver is painfully simple, so we will encode the new format into the
									//	change request arguments. We use the first 32 bit agument to signify that
									//	we're doing a stream format change using the user client method ID. The
									//	second 32 bit argument will hold the stream ID of stream that is changing
									//	format. The first 64 bit argument holds the new sample rate. The second 64
									//	bit argument holds the index into the array of supported formats that
									//	represents the new sample format.
									//requestConfigChange(kIOAudio2UserClientMethodID_ChangeStreamFormat, kOutputStreamID, theSanitizedNewFormat.mSampleRate, theFormatIndex);
									//change the format.
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() change the format");
									// twiddle the hardware here to change format
									mCurrentOutputStreamFormat = theSanitizedNewFormat;
									
									IOVideoStreamDictionary::setCurrentFormat(theStreamDictionary, mCurrentOutputStreamFormat);
									setProperty(kIOVideoDeviceKey_OutputStreamList,theLocalOutputStreamList);
									switch (mCurrentOutputStreamFormat.mVideoCodecType)
									{
										case kYUV422_720x480:
										{
                                            mTimeInterval = kTimerIntervalNTSC;
										}
											break;
										case kYUV422_720x486:
										{
                                             mTimeInterval = kTimerIntervalNTSC;
										}
											break;
										case kYUV422_720x576:
										{
                                            mTimeInterval = kTimerIntervalPAL;
										}
											break;
										case kYUV422_1280x720:
										{
                                            mTimeInterval = CodecFlagsToFrameRate(mCurrentOutputStreamFormat.mVideoCodecFlags);
										}
											break;
										case kYUV422_1920x1080:
										{
                                            mTimeInterval = CodecFlagsToFrameRate(mCurrentOutputStreamFormat.mVideoCodecFlags);
										}
											break;
									}
									
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() mTimeInterval = %d",(int)mTimeInterval);
									
									ResetOutputStreams();
								}
								else
								{
									DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() formats are not different");
								}
								
								//	make sure we return that we were successful
								theAnswer = kIOReturnSuccess;
							}
						}
						else
						{
							DEBUG_LOG("IOVideoSampleDevice::setStreamFormat() no supported formats");
							//	when there are no explicitly supported formats, the stream's format can't be changed
							//	but we should return a bad format error if the new format is different from the current format
							if (IOVideoStreamFormatDictionary::isSameSampleFormat(theNewFormat, mCurrentOutputStreamFormat))
							{
								theAnswer = kIOReturnSuccess;
							}
						}
					}
					theLocalOutputStreamList->release();
				}
			}
		}
	}
	return kIOReturnSuccess; 
}



OSDictionary*	IOVideoSampleDevice::createDefaultInputStreamDictionary()
{
	DEBUG_LOG("IOVideoSampleDevice::createDefaultInputStreamDictionary()");

	//	This method create an output stream dictionary in it's default state
	OSDictionary* theAnswer = NULL;
	OSArray*	  theFormatList = NULL;
	
	//	initialize some locals that might need cleaning up
	OSDictionary*	theCurrentFormat = NULL;

	theFormatList = OSArray::withCapacity(2);
	if (!theFormatList)
	{
		DEBUG_LOG("IOCX88_Video::createDefaultInputStreamDictionary: couldn't allocate format list\n");
		return NULL;
	}

	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1920x1080, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultInputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();

	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1280x720, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultInputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
	
	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_720x480, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 720, 480);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultInputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);

		
	//	create the stream dictionary
	theAnswer = IOVideoStreamDictionary::create(kInputStreamID, 1, theCurrentFormat, theFormatList);
	FailIfNULL(theAnswer, Done, "NullDevice::createDefaultInputStreamDictionary: couldn't allocate the output stream");
	
Done:
	//	clean up what needs cleaning
	if (NULL != theCurrentFormat)
	{
		theCurrentFormat->release();
	}
	if (NULL != theFormatList)
	{
		theFormatList->release();
	}
		
	return theAnswer;
}

OSDictionary*	IOVideoSampleDevice::createDefaultOutputStreamDictionary()
{
	DEBUG_LOG("IOVideoSampleDevice::createDefaultOutputStreamDictionary()");

	//	This method create an output stream dictionary in it's default state
	OSDictionary* theAnswer = NULL;
	OSArray*	  theFormatList = NULL;
	
	//	initialize some locals that might need cleaning up
	OSDictionary*	theCurrentFormat = NULL;



	theFormatList = OSArray::withCapacity(2);
	if (!theFormatList)
	{
		DEBUG_LOG("IOCX88_Video::createDefaultInputStreamDictionary: couldn't allocate format list\n");
		return NULL;
	}

	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1920x1080, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1920x1080, kSampleCodecFlags_30fps, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1920x1080, kSampleCodecFlags_50fps, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1280x720, kSampleCodecFlags_60fps+kSampleCodecFlags_1001_1000_adjust, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1280x720, kSampleCodecFlags_60fps, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_1280x720, kSampleCodecFlags_50fps, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_720x576, kSampleCodecFlags_25fps, 720, 576);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_720x480, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 720, 480);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);

	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1920x1080, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
 
    theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1920x1080, kSampleCodecFlags_30fps, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();

    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1920x1080, kSampleCodecFlags_50fps, 1920, 1080);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1280x720, kSampleCodecFlags_60fps+kSampleCodecFlags_1001_1000_adjust, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
 
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1280x720, kSampleCodecFlags_60fps, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_1280x720, kSampleCodecFlags_50fps, 1280, 720);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_720x486, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 720, 486);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
    
    //	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_720x576, kSampleCodecFlags_25fps, 720, 576);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();

	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_720x480, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 720, 480);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);
	theCurrentFormat->release();
	
	//	make the current format ()
	theCurrentFormat = IOVideoStreamFormatDictionary::create(kYUV422_10_720x486, kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust, 720, 486);
	FailIfNULL(theCurrentFormat, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate current format");
	theFormatList->setObject(theCurrentFormat);

		
	//	create the stream dictionary
	theAnswer = IOVideoStreamDictionary::create(kOutputStreamID, 1, theCurrentFormat, theFormatList);
	FailIfNULL(theAnswer, Done, "NullDevice::createDefaultOutputStreamDictionary: couldn't allocate the output stream");
	
Done:
	//	clean up what needs cleaning
	if (NULL != theCurrentFormat)
	{
		theCurrentFormat->release();
	}
	if (NULL != theFormatList)
	{
		theFormatList->release();
	}
		
	return theAnswer;
}

bool IOVideoSampleDevice::AddInputStreams()
{
	int i;
	IOMemoryDescriptor* descr;
	IOMemoryDescriptor* cntrldescr;
	IOStreamBuffer* buf;
	OSArray* buffers;
	int numBuffers;
	OSDictionary*	theInputStream = NULL;
	OSArray*		theInputStreamList = NULL;
	IOByteCount		theBufferSize;
	char*			srcBuffer;

	DEBUG_LOG("IOVideoSampleDevice::AddInputStreams\n");

	do
	{
		//	create the array that will hold the output streams
		theInputStreamList = OSArray::withCapacity(1);
		
		//	create an output stream dictionary
		theInputStream = createDefaultInputStreamDictionary();
		
		//	keep a local copy of the current format for convenience
		IOVideoStreamDictionary::getCurrentFormat(theInputStream, mCurrentInputStreamFormat);
		
		//	put the output stream into the list
		theInputStreamList->setObject(theInputStream);
		
		//	put the output stream list into the registry
		setProperty(kIOVideoDeviceKey_InputStreamList, theInputStreamList);

		switch (mCurrentInputStreamFormat.mVideoCodecType)
		{
			case kYUV422_720x480:
			{
                srcBuffer = NULL;//mYUVData;
                numBuffers = 30; //mYUVSize / kYUVVFrameSize;
				theBufferSize = kYUVVFrameSize; 
			}
				break;
			case kYUV422_1280x720:
			{
                srcBuffer = NULL; //mHD720pYUVData;
                numBuffers = 30;  //mHD720pYUVSize / kHD720pYUVVFrameSize;
				if (numBuffers > mMaxNumBuffers)
					numBuffers = mMaxNumBuffers;
				theBufferSize = kHD720pYUVVFrameSize; 
				
			}
				break;
			case kYUV422_1920x1080:
			{
                srcBuffer = NULL; //mHD1080pYUVData;
                numBuffers = 30;  //mHD1080pYUVSize / kHD1080pYUVVFrameSize;
				if (numBuffers > mMaxNumBuffers)
					numBuffers = mMaxNumBuffers;
				theBufferSize = kHD1080pYUVVFrameSize; 
				
			}
				break;
		}
		
		mNumBuffers = numBuffers;
		
		buffers = OSArray::withCapacity(numBuffers);
		DEBUG_LOG("num output buffers %d\n", numBuffers);
		
		if (!buffers) break;
		
		for (i=0; i<numBuffers; i++)
		{
			LoadFrameBuffer(mFrameBuf+(i*MAX_FRAME_SIZE), srcBuffer, theBufferSize, i);
            descr = IOMemoryDescriptor::withAddress(mFrameBuf + (i * MAX_FRAME_SIZE),
													theBufferSize,
													kIODirectionOutIn);
			if (!descr)
			{
				DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
				break;
			}

			cntrldescr = IOMemoryDescriptor::withAddress(mControlBuf + (i * sizeof(struct SampleVideoDeviceControlBuffer)),
													sizeof(struct SampleVideoDeviceControlBuffer),
													kIODirectionOutIn);
			if (!cntrldescr)
			{
				DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
				break;
			}
			 
			buf = IOStreamBuffer::withMemoryDescriptors(descr, cntrldescr);
			if (!buf)
			{
				DEBUG_LOG("Couldn't create IOStreamBuffer\n");
				break;
			}
			
			buffers->setObject(buf);
			buf->release();
		}
		
		_currentBuffer = 0;
		
		_outputStream = IOVideoSampleStream::withBuffers(buffers, kIOStreamModeOutput, 0, theInputStream);
		if (!_outputStream)
			break;
		
		// This will attach the stream in the registry
		if (addStream(_outputStream) != kIOReturnSuccess)
			break;
		
		// Buffers are retained by the stream
		buffers->release();
		buffers = NULL;

		//	clean up what needs cleaning
		if (NULL != theInputStream)
		{
			theInputStream->release();
		}
		
		if (NULL != theInputStreamList)
		{
			theInputStreamList->release();
		}


		DEBUG_LOG("AddInputStreams successful\n");
		
		return true;
		
	} while (false);

	if (buffers) buffers->release();
		
	RELEASE_IF_SET (_outputStream);
	
	return false;

}
bool IOVideoSampleDevice::AddOutputStreams()
{
	int i;
	IOMemoryDescriptor* descr;
	IOMemoryDescriptor* cntrldescr;
	IOStreamBuffer* buf;
	OSArray* buffers;
	int numBuffers;
	//	initialize some locals that might need cleaning up
	OSDictionary*	theOutputStream = NULL;
	OSArray*		theOutputStreamList = NULL;
	IOByteCount		theBufferSize;

	DEBUG_LOG("IOVideoSampleDevice::AddOutputStreams\n");
	do
	{
		//	create the array that will hold the output streams
		theOutputStreamList = OSArray::withCapacity(1);
		
		//	create an output stream dictionary
		theOutputStream = createDefaultOutputStreamDictionary();
		
		//	keep a local copy of the current format for convenience
		IOVideoStreamDictionary::getCurrentFormat(theOutputStream, mCurrentOutputStreamFormat);
		
		//	put the output stream into the list
		theOutputStreamList->setObject(theOutputStream);
		
		//	put the output stream list into the registry
		setProperty(kIOVideoDeviceKey_OutputStreamList, theOutputStreamList);


		switch (mCurrentOutputStreamFormat.mVideoCodecType)
		{
			case kYUV422_720x480:
			{
                numBuffers = 30;//mYUVSize / kYUVVFrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kYUVVFrameSize; 
			}
				break;
            case kYUV422_720x486:
            {
                numBuffers = 30;//mYUVSize / kFullNTSCYUVVFrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
                theBufferSize = kFullNTSCYUVVFrameSize; 
                
            }
                break;
            case kYUV422_720x576:
            {
                numBuffers = 30;//mYUVSize / kPALYUVVFrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
                theBufferSize = kPALYUVVFrameSize; 
                
            }
                break;
			case kYUV422_1280x720:
			{
				numBuffers = 30;//mHD720pYUVSize / kHD720pYUVVFrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kHD720pYUVVFrameSize; 
				
			}
				break;
			case kYUV422_1920x1080:
			{
				numBuffers = 30;//mHD1080pYUVSize / kHD1080pYUVVFrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kHD1080pYUVVFrameSize; 
				
			}
				break;
			case kYUV422_10_720x480:
			{
				numBuffers = 30;//mYUVSize / kYUVV_10_FrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kYUVV_10_FrameSize; 
			}
				break;
            case kYUV422_10_720x486:
            {
                numBuffers = 30;//mYUVSize / kFullNTSCYUVV_10_FrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
               theBufferSize = kFullNTSCYUVV_10_FrameSize; 
                
            }
                break;
            case kYUV422_10_720x576:
            {
                numBuffers = 30;//mYUVSize / kPALYUVV_10_FrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
                theBufferSize = kPALYUVV_10_FrameSize; 
                
            }
                break;
			case kYUV422_10_1280x720:
			{
				numBuffers = 30;//mHD720pYUVSize / kHD720pYUVV_10_FrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kHD720pYUVV_10_FrameSize; 
				
			}
				break;
			case kYUV422_10_1920x1080:
			{
				numBuffers = 30;//mHD1080pYUVSize / kHD1080pYUVV_10_FrameSize;
                numBuffers = min(numBuffers,kMaxOutputBuffers);
				theBufferSize = kHD1080pYUVV_10_FrameSize; 
				
			}
				break;
		}
		
		mNumBuffers = numBuffers;

		buffers = OSArray::withCapacity(numBuffers);
		DEBUG_LOG("num output buffers %d\n", numBuffers);
		
		if (!buffers) break;
		
        ZeroControlBuffer(mControlBuf, sizeof(struct SampleVideoDeviceControlBuffer)*mNumBuffers);
        
		for (i=0; i<numBuffers; i++)
		{
			descr = IOMemoryDescriptor::withAddress(mFrameBuf + (i * MAX_FRAME_SIZE),
													theBufferSize,
													kIODirectionOutIn);
			if (!descr)
			{
				DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
				break;
			}

			cntrldescr = IOMemoryDescriptor::withAddress(mControlBuf + (i * sizeof(struct SampleVideoDeviceControlBuffer)),
													sizeof(struct SampleVideoDeviceControlBuffer),
													kIODirectionOutIn);
			if (!cntrldescr)
			{
				DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
				break;
			}

			buf = IOStreamBuffer::withMemoryDescriptors(descr, cntrldescr);
			if (!buf)
			{
				DEBUG_LOG("Couldn't create IOStreamBuffer\n");
				break;
			}
			
			buffers->setObject(buf);
			buf->release();
		}
		
		_currentBuffer = 0;
		
		_inputStream = IOVideoSampleStream::withBuffers(buffers, kIOStreamModeInput, 0, theOutputStream);
		if (!_inputStream) break;
		
		// This will attach the stream in the registry
		if (addStream(_inputStream) != kIOReturnSuccess)
			break;
		
		buffers->release();

		//	clean up what needs cleaning
		if (NULL != theOutputStream)
		{
			theOutputStream->release();
		}

		if (NULL != theOutputStreamList)
		{
			theOutputStreamList->release();
		}
		
		return true;
		
	} while (false);

	if (buffers) buffers->release();
		
	RELEASE_IF_SET (_inputStream);
	
	return false;
}

bool IOVideoSampleDevice::RemoveInputStreams()
{
	DEBUG_LOG("IOVideoSampleDevice::RemoveInputStreams\n");
	do
	{
        removeProperty(kIOVideoDeviceKey_InputStreamList);
        int count, i;
        count = getStreamCount();
        for (i=0; i<count; i++)
        {
            IOVideoStream* stream = NULL;
				
            stream = getStream(i);
            if (NULL != stream)
            {
                if (stream == _outputStream)
                {
                    removeStream(i);
                }
            }
        }
        RELEASE_IF_SET(_outputStream);
        return true;
		
	} while (false);
	
	return false;
}

bool IOVideoSampleDevice::RemoveOutputStreams()
{
	DEBUG_LOG("IOVideoSampleDevice::RemoveOutputStreams\n");
	do
	{
        removeProperty(kIOVideoDeviceKey_OutputStreamList);
        int count, i;
			
        count = getStreamCount();
        for (i=0; i<count; i++)
        {
            IOVideoStream* stream = NULL;
				
            stream = getStream(i);
            if (NULL != stream)
            {
                if (stream == _inputStream)
                {
                    removeStream(i);
                }
            }
        }
        RELEASE_IF_SET(_inputStream);
		
        return true;
		
	} while (false);
	
	return false;
}

bool IOVideoSampleDevice::ResetInputStreams()
{
	OSArray* buffers;
	IOMemoryDescriptor* descr, * controlDescr;
	int i;
	IOStreamBuffer* buf;
	IOByteCount		bufferSize;
	int numBuffers;
	char*	srcBuffer;
	IOReturn removeStatus = kIOReturnSuccess;  
	
	if (_outputStream)
	{
		do
		{
			removeStatus = _outputStream->removeAllBuffers();
			DEBUG_LOG("removeAllBuffers status= %x\n", removeStatus);
			
			switch (mCurrentInputStreamFormat.mVideoCodecType)
			{
				case kYUV422_720x480:
				{
                    srcBuffer = NULL;//mYUVData;
					numBuffers = 30;//mYUVSize / kYUVVFrameSize;
					bufferSize = kYUVVFrameSize; 
				}
					break;
				case kYUV422_1280x720:
				{
                    srcBuffer = NULL;//mHD720pYUVData;
					numBuffers = 30;//mHD720pYUVSize / kHD720pYUVVFrameSize;
					if (numBuffers > mMaxNumBuffers)
						numBuffers = mMaxNumBuffers;
					bufferSize = kHD720pYUVVFrameSize; 
					
				}
					break;
				case kYUV422_1920x1080:
				{
                    srcBuffer = NULL;//mHD1080pYUVData;
					numBuffers = 30;//mHD1080pYUVSize / kHD1080pYUVVFrameSize;
					if (numBuffers > mMaxNumBuffers)
						numBuffers = mMaxNumBuffers;
					bufferSize = kHD1080pYUVVFrameSize; 
					
				}
					break;
			}
			
			mNumBuffers = numBuffers;
			
			DEBUG_LOG("bufferSize = %ld\n", (long int)bufferSize);
			
			
			buffers = OSArray::withCapacity(mNumBuffers);
			if (!buffers) 
			{
				DEBUG_LOG("OSArray::withCapacity - buffers failed\n");
				break;
			}
			
			for (i=0; i<mNumBuffers; i++) 
			{
				LoadFrameBuffer(mFrameBuf+(i*MAX_FRAME_SIZE), srcBuffer, bufferSize, i);
				descr = IOMemoryDescriptor::withAddress(mFrameBuf+(i * MAX_FRAME_SIZE),
														bufferSize,
														kIODirectionOutIn);
				if (!descr) 
				{
					DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
					return false;
				}
				
				controlDescr = IOMemoryDescriptor::withAddress(mControlBuf+(i * sizeof(struct SampleVideoDeviceControlBuffer)),
															   sizeof(struct SampleVideoDeviceControlBuffer),
															   kIODirectionOutIn);
				if (!controlDescr) 
				{
					DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
					return false;
				}
				
				
				buf = IOStreamBuffer::withMemoryDescriptors(descr, controlDescr);
				if (!buf) 
				{
					DEBUG_LOG("Couldn't create IOStreamBuffer\n");
					return false;
				}
				
				buffers->setObject(buf);
				buf->release();
			}
			
			_outputStream->addBuffers(buffers);
			
			buffers->release();
			
			return true;
			
		}while(false);
		
		if (buffers) buffers->release();
		
	}
	return false;
}

bool IOVideoSampleDevice::ResetOutputStreams()
{
	OSArray* buffers;
	IOMemoryDescriptor* descr, *controlDescr;
	int i;
	IOStreamBuffer* buf;
	IOByteCount		bufferSize;
	int numBuffers;
	
	if (_inputStream)
	{
		do
		{
			_inputStream->removeAllBuffers();

			switch (mCurrentOutputStreamFormat.mVideoCodecType)
			{
				case kYUV422_720x480:
				{
					numBuffers = 30;//mYUVSize / kYUVVFrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
					bufferSize = kYUVVFrameSize; 
				}
					break;
                case kYUV422_720x486:
                {
					numBuffers = 30;//mYUVSize / kFullNTSCYUVVFrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
					bufferSize = kFullNTSCYUVVFrameSize; 
                    
                }
                    break;
                case kYUV422_720x576:
                {
					numBuffers = 30;//mYUVSize / kPALYUVVFrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
					bufferSize = kPALYUVVFrameSize; 
                    
                }
                    break;
				case kYUV422_1280x720:
				{
					numBuffers = 30;//mHD720pYUVSize / kHD720pYUVVFrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
					bufferSize = kHD720pYUVVFrameSize; 
					
				}
					break;
				case kYUV422_1920x1080:
				{
					numBuffers = 30;//mHD1080pYUVSize / kHD1080pYUVVFrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
					bufferSize = kHD1080pYUVVFrameSize; 
					
				}
					break;
                case kYUV422_10_720x480:
                {
                    numBuffers = 30;//mYUVSize / kYUVV_10_FrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
                   bufferSize = kYUVV_10_FrameSize; 
                }
                    break;
                case kYUV422_10_720x486:
                {
                    numBuffers = 30;//mYUVSize / kFullNTSCYUVV_10_FrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
                   bufferSize = kFullNTSCYUVV_10_FrameSize; 
                    
                }
                    break;
                case kYUV422_10_720x576:
                {
                    numBuffers = 30;//mYUVSize / kPALYUVV_10_FrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
                    bufferSize = kPALYUVV_10_FrameSize; 
                    
                }
                    break;
                case kYUV422_10_1280x720:
                {
                    numBuffers = 30;//mHD720pYUVSize / kHD720pYUVV_10_FrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
                    bufferSize = kHD720pYUVV_10_FrameSize; 
                    
                }
                    break;
                case kYUV422_10_1920x1080:
                {
                    numBuffers = 30;//mHD1080pYUVSize / kHD1080pYUVV_10_FrameSize;
                    numBuffers = min(numBuffers,kMaxOutputBuffers);
                    bufferSize = kHD1080pYUVV_10_FrameSize; 
                    
                }
                    break;
			}
			
			mNumBuffers = numBuffers;
			
			DEBUG_LOG("ResetOutputStreams bufferSize = %ld\n", (long int)bufferSize);
			buffers = OSArray::withCapacity(mNumBuffers);
			if (!buffers) 
			{
				DEBUG_LOG("OSArray::withCapacity - buffers failed\n");
				break;
			}
			
			for (i=0; i<mNumBuffers; i++) 
			{
				descr = IOMemoryDescriptor::withAddress(mFrameBuf+(i * MAX_FRAME_SIZE),
														bufferSize,
														kIODirectionOutIn);
				if (!descr) 
				{
					DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
					return false;
				}
				
				controlDescr = IOMemoryDescriptor::withAddress(mControlBuf+(i * sizeof(struct SampleVideoDeviceControlBuffer)),
															   sizeof(struct SampleVideoDeviceControlBuffer),
															   kIODirectionOutIn);
				if (!controlDescr) 
				{
					DEBUG_LOG("Couldn't create IOBufferMemoryDescriptor\n");
					return false;
				}
				
				
				buf = IOStreamBuffer::withMemoryDescriptors(descr, controlDescr);
				if (!buf) 
				{
					DEBUG_LOG("Couldn't create IOStreamBuffer\n");
					return false;
				}
				
				buffers->setObject(buf);
				buf->release();
			}
			
			_inputStream->addBuffers(buffers);
			
			buffers->release();
			
			return true;
			
		}while(false);
		
		if (buffers) buffers->release();
		
	}
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	AllocateFrameBuffers
//--------------------------------------------------------------------------------------------------------------------
IOReturn IOVideoSampleDevice::AllocateFrameBuffers(size_t size)
{	
	DEBUG_LOG("IOVideoSampleDevice::AllocateDataBuffers\n");
	
	// allocate the memory
	#if TARGET_CPU_X86
			mMemDescFrameBuf = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
										kernel_task,							// task to hold mem
										kIOMemoryKernelUserShared,				// options
										size,									// size
										0x00000000FFFFF000ULL);					// physicalMask - 32 bit addressable and page aligned
	#else
		mMemDescFrameBuf = IOBufferMemoryDescriptor::inTaskWithOptions( 
										kernel_task,							// task to hold mem
										kIOMemoryKernelUserShared,				// options
										size,									// size
										page_size);								// alignment
	#endif
										 
	if (NULL == mMemDescFrameBuf)
	{
		DEBUG_LOG("IOVideoSampleDevice::AllocateFrameBuffers Unable to allocate memory desc\n");
		return kIOReturnNoMemory;
	}
	
	// map it
	mMemMapFrameBuf = mMemDescFrameBuf->map() ;
	if (NULL == mMemMapFrameBuf)
	{
		DEBUG_LOG("IOVideoSampleDevice::AllocateFrameBuffers Unable to obtain memory map\n");
		return kIOReturnNoMemory;
	}

	// get the virtual address and initialize the allocator
	mFrameBuf = (UInt8*)mMemMapFrameBuf->getVirtualAddress() ;
	mFrameBufSize = size;
	
	return kIOReturnSuccess;
}


//--------------------------------------------------------------------------------------------------------------------
//	AllocateControlBuffers
//--------------------------------------------------------------------------------------------------------------------
IOReturn IOVideoSampleDevice::AllocateControlBuffers(size_t size)
{	
	DEBUG_LOG("IOVideoSampleDevice::AllocateControlBuffers\n");

	// allocate the memory
	#if TARGET_CPU_X86
			mMemDescControlBuf = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
										kernel_task,							// task to hold mem
										kIOMemoryKernelUserShared,				// options
										size,									// size
										0x00000000FFFFF000ULL);					// physicalMask - 32 bit addressable and page aligned
	#else
		mMemDescControlBuf = IOBufferMemoryDescriptor::inTaskWithOptions(	
										kernel_task,							// task to hold mem
										kIOMemoryKernelUserShared,				// options
										size,									// size
										page_size);								// alignment
	#endif
										 
	if (NULL == mMemDescControlBuf)
	{
		DEBUG_LOG("IOVideoSampleDevice::AllocateControlBuffers Unable to allocate memory desc\n");
		return kIOReturnNoMemory;
	}
	
	// map it
	mMemMapControlBuf = mMemDescControlBuf->map() ;
	if (NULL == mMemMapControlBuf)
	{
		DEBUG_LOG("IOVideoSampleDevice::AllocateControlBuffers Unable to obtain memory map\n");
		return kIOReturnNoMemory;
	}

	// get the virtual address and initialize the allocator
	mControlBuf = (UInt8*)mMemMapControlBuf->getVirtualAddress() ;
	mControlBufSize = size;
	
	return kIOReturnSuccess;
}

//--------------------------------------------------------------------------------------------------------------------
//	LoadFrameBuffer
//--------------------------------------------------------------------------------------------------------------------
void IOVideoSampleDevice::LoadFrameBuffer(void* buffer, char* src, IOByteCount frameSize, SInt32 number)
{
    char* temp = (char*)buffer;
    for (int i = 0; i < frameSize; ++i) {
        temp[i] = 255;
    }
//	bcopy(temp, buffer, frameSize);
}

void IOVideoSampleDevice::ZeroControlBuffer(void* buffer, IOByteCount bufSize)
{
    bzero(buffer,bufSize);	
}

UInt32 IOVideoSampleDevice::CodecFlagsToFrameRate(UInt32 codecFlags)
{
    UInt32 frameRate= kTimerIntervalNTSC;
    
    switch(codecFlags)
    {
        case kSampleCodecFlags_60fps:
        {
            frameRate = kTimerIntervalHD60; 
        }
            break;
        case kSampleCodecFlags_60fps+kSampleCodecFlags_1001_1000_adjust:
        {
            frameRate = kTimerIntervalHD5994; 
        }
            break;
        case kSampleCodecFlags_50fps:
        {
            frameRate = kTimerIntervalHD50; 
        }
            break;
        case kSampleCodecFlags_30fps:
        {
            frameRate = kTimerInterval30; 
        }
            break;
        case kSampleCodecFlags_30fps+kSampleCodecFlags_1001_1000_adjust:
        {
            frameRate = kTimerIntervalNTSC; 
        }
            break;
        case kSampleCodecFlags_25fps:
        {
            frameRate = kTimerIntervalPAL; 
        }
            break;
        case kSampleCodecFlags_24fps:
        {
            frameRate = kTimerInterval24; 
        }
            break;
        case kSampleCodecFlags_24fps+kSampleCodecFlags_1001_1000_adjust:
        {
            frameRate = kTimerInterval2397; 
        }
            break;
    }
    DEBUG_LOG("IOVideoSampleDevice::CodecFlagsToFrameRate rate = %d\n",(int)frameRate);
    return frameRate; 
}

