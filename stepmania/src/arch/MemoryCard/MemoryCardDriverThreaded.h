#ifndef MemoryCardDriverThreaded_H
#define MemoryCardDriverThreaded_H 1

#include "MemoryCardDriver.h"
#include "RageThreads.h"

struct UsbStorageDeviceEx : public UsbStorageDevice
{
	UsbStorageDeviceEx() { MakeBlank(); }

	void MakeBlank()
	{
		bWriteTestSucceeded = false;
	}
	
	bool bWriteTestSucceeded;
};

class MemoryCardDriverThreaded : public MemoryCardDriver
{
public:
	MemoryCardDriverThreaded();
	~MemoryCardDriverThreaded();

	virtual bool StorageDevicesChanged();
	virtual void GetStorageDevices( vector<UsbStorageDevice>& vStorageDevicesOut );
	virtual bool MountAndTestWrite( UsbStorageDevice* pDevice, CString sMountPoint );
    virtual void ResetUsbStorage();
	virtual void PauseMountingThread();
	virtual void UnPauseMountingThread();

private:
	static int MountThread_Start( void *p );
	void MountThreadMain();

	RageThread m_threadMemoryCardMount;
	bool m_bShutdownNextUpdate;
	bool m_bResetNextUpdate;

	// Aquire this before detecting devices or reading/writing devices.
	// Calling Pause() and Unpause will lock/unlock this so that the mounting thread 
	// will temporarily halt.
	RageMutex m_mutexPause;

protected:
	void StartThread();	// call this in the derived constructor to start the mounting thread
	virtual void MountThreadReset() = 0;
	virtual void MountThreadDoOneUpdate() = 0;	// this will get called as fast as possible
	virtual void Mount( UsbStorageDevice* pDevice, CString sMountPoint ) = 0;

	vector<UsbStorageDeviceEx> m_vStorageDevices;
	bool m_bStorageDevicesChanged;
	RageMutex m_mutexStorageDevices;	// protects the above two
};

#endif

/*
 * (c) 2003-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
