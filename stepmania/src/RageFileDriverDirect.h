#ifndef RAGE_FILE_DRIVER_DIRECT_H
#define RAGE_FILE_DRIVER_DIRECT_H

#include "RageFileDriver.h"

class RageFileDriverDirect: public RageFileDriver
{
public:
	RageFileDriverDirect( CString root );

	RageFileObj *Open( const CString &path, int mode, RageFile &p, int &err );
	bool Remove( const CString &sPath );
	bool Ready();

private:
	CString root;
};

#endif
