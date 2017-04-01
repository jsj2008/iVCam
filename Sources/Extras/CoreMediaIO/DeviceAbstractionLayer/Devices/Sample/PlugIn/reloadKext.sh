#!/bin/sh

/bin/sleep 90

/sbin/kextunload "/Library/Extensions/IOVideoSample.kext"
/sbin/kextload "/Library/Extensions/IOVideoSample.kext"
