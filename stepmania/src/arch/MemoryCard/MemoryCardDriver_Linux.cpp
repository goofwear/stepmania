#include "global.h"
#include "MemoryCardDriver_Linux.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageFileManager.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>

#include <fstream>

static const char *USB_DEVICE_LIST_FILE = "/proc/bus/usb/devices";
static const char *ETC_MTAB = "/etc/mtab";

int MemoryCardDriver_Linux::MountThread_Start( void *p )
{
  ((MemoryCardDriver_Linux*)p)->MountThreadMain();
  return 0;
}

template<class T>
bool VectorsAreEqual( const T &a, const T &b )
{
  if( a.size() != b.size() )
      return false;

  for( unsigned i=0; i<a.size(); i++ )
    {
      if( a[i] != b[i] )
	return false;
    }

  return true;
}

void MemoryCardDriver_Linux::MountThreadMain()
{
  int fd = open(USB_DEVICE_LIST_FILE, O_RDONLY);
  if( fd == -1 )
    {
      LOG->Warn( "Failed to open \"%s\": %s", USB_DEVICE_LIST_FILE, strerror(errno) );
      return;
    }

  time_t lastModTime = 0;

  vector<UsbStorageDevice> vDevicesLastSeen;

  while( !shutdown )
    {      
      struct stat st;
      if( fstat(fd, &st) == -1 )
        {
	  LOG->Warn( "stat '%s' failed: %s", USB_DEVICE_LIST_FILE, strerror(errno) );
	  close( fd );
	  return;
        }

      bool bChanged = st.st_mtime != lastModTime;
      lastModTime = st.st_mtime;

      if( bChanged )
	{
	  // TRICKY: We're waiting for a change in the USB device list, but 
	  // the usb-storage descriptors take a bit longer to update.  It's more convenient to wait
	  // on the USB device list because the usb-storage descriptors are separate files per 
	  // device.  So, sleep for a little bit of time after we detect a new USB device and give
	  // usb-storage a chance to initialize.  
	  usleep(1000*300);

	  vector<UsbStorageDevice> vDevicesNow;
	  this->GetStorageDevices( vDevicesNow );

	  vector<UsbStorageDevice> &vNew = vDevicesNow;
	  vector<UsbStorageDevice> &vOld = vDevicesLastSeen;

	  // check for disconnects
	  vector<UsbStorageDevice> vDisconnects;
	  for( unsigned i=0; i<vOld.size(); i++ )
	    {
	      const UsbStorageDevice old = vOld[i];
	      if( find(vNew.begin(),vNew.end(),old) == vNew.end() )// didn't find
		{
		  LOG->Trace( ssprintf("Disconnected bus %d port %d device %d path %s", old.iBus, old.iPortOnHub, old.iDeviceOnBus, old.sOsMountDir.c_str()) );
		  vDisconnects.push_back( old );
		}
	    }

	  // check for connects
          vector<UsbStorageDevice> vConnects;
	  for( unsigned i=0; i<vNew.size(); i++ )
	    {
	      const UsbStorageDevice newd = vNew[i];
	      if( find(vOld.begin(),vOld.end(),newd) == vOld.end() )// didn't find
		{
		  LOG->Trace( ssprintf("Connected bus %d port %d device %d path %s", newd.iBus, newd.iPortOnHub, newd.iDeviceOnBus, newd.sOsMountDir.c_str()) );
		  vConnects.push_back( newd );
		}
	    }

	  // unmount all disconnects
	  for( unsigned i=0; i<vDisconnects.size(); i++ )
	    {
	      const UsbStorageDevice &d = vDisconnects[i];
	      CString sCommand = "umount " + d.sOsMountDir;
	      LOG->Trace( "executing '%s'", sCommand.c_str() );
	      if( system(sCommand) == -1 )
		LOG->Warn( "failed to execute '%s'", sCommand.c_str() );
	    }
	  
	  // mount all connects
	  for( unsigned i=0; i<vConnects.size(); i++ )
	    {	  
	      const UsbStorageDevice d = vConnects[i];
	      CString sCommand;
	      
	      // unmount this device before trying to mount it.  If this device
	      // wasn't unmounted before, then our mount call will fail and the 
	      // mount may contain an out-of-date view of the files on the device.
              sCommand = "umount " + d.sOsMountDir;
              LOG->Trace( "executing '%s'", sCommand.c_str() );
              if( system(sCommand) == -1 )
                LOG->Warn( "failed to execute '%s'", sCommand.c_str() );

	      sCommand = "mount " + d.sOsMountDir;
	      LOG->Trace( "executing '%s'", sCommand.c_str() );
	      if( system(sCommand) == -1 )
		LOG->Warn( "failed to execute '%s'", sCommand.c_str() );
	    }

	  if( !vDisconnects.empty() || !vConnects.empty() )	  
	    {
	      LockMut( m_StorageDevicesChangedMutex );
	      m_bStorageDevicesChanged = true;
	    }

	  vDevicesLastSeen = vDevicesNow;
	}
      usleep( 1000*100 );  // 100 ms
    }
  CHECKPOINT;
}

MemoryCardDriver_Linux::MemoryCardDriver_Linux() :
  m_StorageDevicesChangedMutex("StorageDevicesChanged")
{
	shutdown = false;
	m_bStorageDevicesChanged = false;

	MountThread.SetName("MemCard Mount thread");
       	MountThread.Create( MountThread_Start, this );
}

MemoryCardDriver_Linux::~MemoryCardDriver_Linux()
{
     shutdown = true;
     LOG->Trace( "Shutting down Mount thread..." );
     MountThread.Wait();
     LOG->Trace( "Mount thread shut down." );
}

bool MemoryCardDriver_Linux::StorageDevicesChanged()
{
  LockMut( m_StorageDevicesChangedMutex );
  if( m_bStorageDevicesChanged )
    {
      m_bStorageDevicesChanged = false;
      return true;
    }
  else
    {
      return false;
    }
}

void MemoryCardDriver_Linux::GetStorageDevices( vector<UsbStorageDevice>& vDevicesOut )
{
	vDevicesOut.clear();

	{
		// Find all attached USB devices.  Output looks like:

		// T:  Bus=02 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#=  1 Spd=12  MxCh= 2
		// B:  Alloc=  0/900 us ( 0%), #Int=  0, #Iso=  0
		// D:  Ver= 1.00 Cls=09(hub  ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
		// P:  Vendor=0000 ProdID=0000 Rev= 0.00
		// S:  Product=USB UHCI Root Hub
		// S:  SerialNumber=ff80
		// C:* #Ifs= 1 Cfg#= 1 Atr=40 MxPwr=  0mA
		// I:  If#= 0 Alt= 0 #EPs= 1 Cls=09(hub  ) Sub=00 Prot=00 Driver=hub
		// E:  Ad=81(I) Atr=03(Int.) MxPS=   8 Ivl=255ms
		// T:  Bus=02 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#=  2 Spd=12  MxCh= 0
		// D:  Ver= 1.10 Cls=00(>ifc ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
		// P:  Vendor=04e8 ProdID=0100 Rev= 0.01
		// S:  Manufacturer=KINGSTON     
		// S:  Product=USB DRIVE    
		// S:  SerialNumber=1125198948886
		// C:* #Ifs= 1 Cfg#= 1 Atr=80 MxPwr= 90mA
		// I:  If#= 0 Alt= 0 #EPs= 2 Cls=08(stor.) Sub=06 Prot=50 Driver=usb-storage
		// E:  Ad=82(I) Atr=02(Bulk) MxPS=  64 Ivl=0ms
		// E:  Ad=03(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms

		ifstream f;
		CString fn = "/proc/bus/usb/devices";
		f.open(fn);
		if( !f.is_open() )
		{
			LOG->Warn( "can't open '%s'", fn.c_str() );
			return;
		}

		UsbStorageDevice usbd;
		CString sLine;
		while( getline(f, sLine) )
		{
			int iRet, iThrowAway;

			// T:  Bus=02 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#=  1 Spd=12  MxCh= 2
			int iBus, iPort, iDevice;
			iRet = sscanf( sLine.c_str(), "T:  Bus=%d Lev=%d Prnt=%d Port=%d Cnt=%d Dev#=%d Spd=%d  MxCh=%d", &iBus, &iThrowAway, &iThrowAway, &iPort, &iThrowAway, &iDevice, &iThrowAway, &iThrowAway );
			if( iRet == 8 )
			{
				usbd.iBus = iBus;
				usbd.iDeviceOnBus = iDevice;
				usbd.iPortOnHub = iPort;
				continue;	// stop processing this line
			}

			// S:  SerialNumber=ff80
			char szSerial[1024];
			iRet = sscanf( sLine.c_str(), "S:  SerialNumber=%[^\n]", szSerial );
			if( iRet == 1 )
			{
				usbd.sSerial = szSerial;
				continue;	// stop processing this line
			}
			
			// I:  If#= 0 Alt= 0 #EPs= 2 Cls=08(stor.) Sub=06 Prot=50 Driver=usb-storage
			int iClass;
			iRet = sscanf( sLine.c_str(), "I:  If#=%d Alt=%d #EPs=%d Cls=%d", &iThrowAway, &iThrowAway, &iThrowAway, &iClass );
			if( iRet == 4 )
			{
				if( iClass == 8 )	// storage class
				{
					vDevicesOut.push_back( usbd );
					LOG->Trace( "iUsbStorageIndex: %d, iBus: %d, iDeviceOnBus: %d, iPortOnHub: %d",
						usbd.iUsbStorageIndex, usbd.iBus, usbd.iDeviceOnBus, usbd.iPortOnHub );
				}
				continue;	// stop processing this line
			}
		}
	}

	{
		// Find the usb-storage device index for all storage class devices.
		for( unsigned i=0; true; i++ )
		{
			// Read the usb-storage descriptor.  It looks like:

			//    Host scsi0: usb-storage
			//        Vendor: KINGSTON     
			//       Product: USB DRIVE    
			// Serial Number: 1125198948886
			//      Protocol: Transparent SCSI
			//     Transport: Bulk
			//          GUID: 04e801000001125198948886
			//      Attached: Yes
	
			CString fn = ssprintf( "/proc/scsi/usb-storage-%d/%d", i, i );
			ifstream f;
			f.open(fn);
			if( !f.is_open() )
				break;

			CString sLine;
			while( getline(f, sLine) )
			{
				// Serial Number: 1125198948886
				char szSerial[1024];
				int iRet = sscanf( sLine.c_str(), "Serial Number: %[^\n]", szSerial );
				if( iRet == 1 )	// we found our line
				{
					// Search for the device corresponding to this serial number.
					for( unsigned j=0; j<vDevicesOut.size(); j++ )
					{
						UsbStorageDevice& usbd = vDevicesOut[j];

						if( usbd.sSerial == szSerial )
						{
							usbd.iUsbStorageIndex = i;
							LOG->Trace( "iUsbStorageIndex: %d, iBus: %d, iDeviceOnBus: %d, iPortOnHub: %d, sSerial: %s",
								usbd.iUsbStorageIndex, usbd.iBus, usbd.iDeviceOnBus, usbd.iPortOnHub, usbd.sSerial.c_str() );
							break;	// done looking for the corresponding device.
						}
					}
					break;	// we already found the line we care about
				}
			}
		}
	}

	{
		// Find where each device is mounted. Output looks like:

		// /dev/sda1               /mnt/flash1             auto    noauto,owner 0 0
		// /dev/sdb1               /mnt/flash2             auto    noauto,owner 0 0
		// /dev/sdc1               /mnt/flash3             auto    noauto,owner 0 0

		CString fn = "/etc/fstab";
		RageFile f;
		if( !f.Open(fn) )
		{
			LOG->Warn( "can't open '%s': %s", fn.c_str(), f.GetError().c_str() );
			return;
		}

		CString sLine;
		while( f.GetLine(sLine) )
		{
			// /dev/sda1               /mnt/flash1             auto    noauto,owner 0 0
			char cScsiDev;
			char szMountPoint[1024];
			int iRet = sscanf( sLine.c_str(), "/dev/sd%c1 %s", &cScsiDev, szMountPoint );
			if( iRet != 2 )
				continue;	// don't process this line

			int iUsbStorageIndex = cScsiDev - 'a';
			CString sMountPoint = szMountPoint;
			TrimLeft( sMountPoint );
			TrimRight( sMountPoint );

			// search for the usb-storage device corresponding to the SCSI device
			for( unsigned i=0; i<vDevicesOut.size(); i++ )
			{
				UsbStorageDevice& usbd = vDevicesOut[i];
				if( usbd.iUsbStorageIndex == iUsbStorageIndex )	// found our match
				{
					usbd.sOsMountDir = sMountPoint;

					LOG->Trace( "iUsbStorageIndex: %d, iBus: %d, iDeviceOnBus: %d, iPortOnHub: %d, sOsMountDir: %s",
						usbd.iUsbStorageIndex, usbd.iBus, usbd.iDeviceOnBus, usbd.iPortOnHub, usbd.sOsMountDir.c_str() );

					break;	// stop looking for a match
				}
			}
		}
	}
}

bool MemoryCardDriver_Linux::MountAndTestWrite( UsbStorageDevice* pDevice, CString sMountPoint )
{
	if( pDevice->sOsMountDir.empty() )
		return false;

	// Already mounted by the mount thread
	//CString sCommand = "umount " + pDevice->sOsMountDir;
	//LOG->Trace( "executing '%s'", sCommand.c_str() );
	//system( sCommand );
	//sCommand = "mount " + pDevice->sOsMountDir;
	//LOG->Trace( "executing '%s'", sCommand.c_str() );
	//system( sCommand );

	// Try to write a file.
	// TODO: Can we use RageFile for this?
	CString sFile = pDevice->sOsMountDir;
	if( sFile[sFile.length()-1] != '/' )
		sFile += '/';
	sFile += "temp";
	FILE* fp = fopen( sFile, "w" );
	if( fp == NULL )
		return false;
	fclose( fp );
	remove( sFile );

	/* Unmount any previous mounts for this mountpoint. */
	vector<RageFileManager::DriverLocation> Mounts;
	FILEMAN->GetLoadedDrivers( Mounts );
	for( unsigned i = 0; i < Mounts.size(); ++i )
	{
		if( Mounts[i].Type.CompareNoCase( "dir" ) )
			continue; // wrong type
		if( Mounts[i].Root.CompareNoCase( pDevice->sOsMountDir ) )
			continue; // wrong root
		FILEMAN->Unmount( Mounts[i].Type, Mounts[i].Root, Mounts[i].MountPoint );
	}

	FILEMAN->Mount( "dir", pDevice->sOsMountDir, sMountPoint.c_str() );
	LOG->Trace( "FILEMAN->Mount %s %s", pDevice->sOsMountDir.c_str(), sMountPoint.c_str() );

	return true;
}

void MemoryCardDriver_Linux::Unmount( UsbStorageDevice* pDevice, CString sMountPoint )
{
	if( pDevice->sOsMountDir.empty() )
		return;

	// already unmounted by the mounting thread
	//CString sCommand = "umount " + pDevice->sOsMountDir;
	//LOG->Trace( "executing '%s'", sCommand.c_str() );
	//system( sCommand );
}

void MemoryCardDriver_Linux::Flush( UsbStorageDevice* pDevice )
{
	if( pDevice->sOsMountDir.empty() )
		return;

	// "sync" will only flush all file systems at the same time.  -Chris
	// I don't think so.  Also, sync() merely queues a flush; it doesn't guarantee
	// that the flush is completed on return.  However, we can mount the filesystem
	// with the flag "-o sync", which forces synchronous access (but that's probably
	// very slow.) -glenn
	CString sCommand = "mount -o remount " + pDevice->sOsMountDir;
	LOG->Trace( "executing '%s'", sCommand.c_str() );
	if( system(sCommand) == -1 )
	  LOG->Warn( "failed to execute '%s'", sCommand.c_str() );

}

void MemoryCardDriver_Linux::ResetUsbStorage()
{
  CString sCommand;
  sCommand = "rmmod usb-storage";
  if( system(sCommand) == -1 )
    LOG->Warn( "failed to execute '%s'", sCommand.c_str() );
  sCommand = "modprobe usb-storage";
  if( system(sCommand) == -1 )
    LOG->Warn( "failed to execute '%s'", sCommand.c_str() );
}


/*
 * Copyright (c) 2003 by the person(s) listed below.  All rights reserved.
 *	Chris Danford
 */
