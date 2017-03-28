### iVCam - A virtual camera for macOS ###

## WHAT IS IT:
By the present of CoreMediaIO framework, we can create virtual camera for some purposes. Many applications use virtual camera to handle camera video stream such as **ManyCam**, **CamTwist**, **CamMask** and etc.

## HOW TO MAKE IT WORK:
* Download and compile the project. 
* Move **\*.plugin to /Library/CoreMediaIO/Plug-Ins/DAL/**, move **\*.kext to /System/Library/Extensions/**.
* Load the kernel extension manually using following commands:

```
$ sudo chown -R root:wheel IOVideoSample.kext      
$ sudo kextunload IOVideoSample.kext
$ sudo kextload IOVideoSample.kext

```
* Open **OBS Studio** or other client application to preview virtual camera.

## WHAT IS COREMEDIAIO:

The CoreMediaIO Device Abstraction Layer (DAL) is analogous to CoreAudio’s Hardware Abstraction Layer (HAL). Just as the HAL deals with audio streams from audio hardware, the DAL handles video (and muxed) streams from video devices.
This SDK will demonstrate how to create a user-level DAL plugIn, a user-level “assistant” server process that allows the device to vend its video data to several processes at once, and a kernel extension (KEXT) for manipulating the device’s hardware.

## BUILD REQUIREMENTS:

Mac OS X v10.7.4 or later

## RUNTIME REQUIREMENTS:

Mac OS X v10.7.4 or later  

## Copyright
Copyright (C) 2012 Apple Inc. All rights reserved.
