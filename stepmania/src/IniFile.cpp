#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: IniFile

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Adam Clauss
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "IniFile.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageFile.h"

IniFile::IniFile(CString inipath)
{
	path = inipath;
}

IniFile::~IniFile()
{

}

void IniFile::SetPath(CString newpath)
{
	path = newpath;
}

// reads ini file specified using IniFile::SetPath()
// returns true if successful, false otherwise
bool IniFile::ReadFile()
{
	CHECKPOINT_M( ssprintf("Reading '%s'",path.c_str()) );

	RageFile f;
	if( !f.Open( path ) )
	{
		LOG->Trace( "Reading '%s' failed: %s", path.c_str(), f.GetError().c_str() );
		error = ssprintf("Unable to open ini file: %s", f.GetError().c_str() );
		return 0;
	}

	CString keyname;
	CString line;
	while( f.GetLine(line) )
	{
		if(line.size() >= 3 &&
			line[0] == '\xef' &&
			line[1] == '\xbb' &&
			line[2] == '\xbf'
			)
		{
			/* Obnoxious NT marker for UTF-8.  Remove it. */
			line.erase(0, 3);
		}

		if (line == "")
			continue;

		if (line.substr(0, 2) == "//" || line.substr(0) == "#")
			continue; /* comment */

		if (line[0] == '[' && line[line.GetLength()-1] == ']') //if a section heading
		{
			keyname = line.substr(1, line.size()-2);
		}
		else //if a value
		{
			int iEqualIndex = line.Find("=");
			if( iEqualIndex != -1 )
			{
				CString valuename = line.Left(iEqualIndex);
				CString value = line.Right(line.GetLength()-valuename.GetLength()-1);
				SetValue(keyname,valuename,value);
			}
		}
	}

	return true;
}

// writes data stored in class to ini file
void IniFile::WriteFile()
{
	RageFile f;
	if( !f.Open( path, RageFile::WRITE ) )
	{
		LOG->Trace( "Writing '%s' failed: %s", path.c_str(), f.GetError().c_str() );
		error = ssprintf("Unable to open ini file for writing: %s", f.GetError().c_str() );
		return;
	}

	for (keymap::const_iterator k = keys.begin(); k != keys.end(); ++k)
	{
		if (k->second.empty())
			continue;

		f.PutLine( ssprintf("[%s]", k->first.c_str()) );

		for (key::const_iterator i = k->second.begin(); i != k->second.end(); ++i)
			f.PutLine( ssprintf("%s=%s\n", i->first.c_str(), i->second.c_str()) );

		f.PutLine( "" );
	}
}

// deletes all stored ini data
void IniFile::Reset()
{
	keys.clear();
}

// returns number of keys currently in the ini
int IniFile::GetNumKeys() const
{
	return keys.size();
}

// returns number of values stored for specified key, or -1 if key not found
int IniFile::GetNumValues(const CString &keyname) const
{
	keymap::const_iterator k = keys.find(keyname);
	if (k == keys.end())
		return -1;

	return k->second.size();
}

// gets value of [keyname] valuename = 
bool IniFile::GetValue(const CString &keyname, const CString &valuename, CString& value) const
{
	keymap::const_iterator k = keys.find(keyname);
	if (k == keys.end())
	{
		error = "Unable to locate specified key.";
		return false;
	}

	key::const_iterator i = k->second.find(valuename);

	if (i == k->second.end())
	{
		error = "Unable to locate specified value.";
		return false;
	}

	value = i->second;
	return true;
}

// gets value of [keyname] valuename = 
bool IniFile::GetValue(const CString &keyname, const CString &valuename, int& value) const
{
	CString sValue;
	if( !GetValue(keyname,valuename,sValue) )
		return false;
	sscanf( sValue.c_str(), "%d", &value );
	return true;
}

bool IniFile::GetValue(const CString &keyname, const CString &valuename, unsigned &value) const
{
	CString sValue;
	if( !GetValue(keyname,valuename,sValue) )
		return false;
	sscanf( sValue.c_str(), "%u", &value );
	return true;
}

// gets value of [keyname] valuename = 
bool IniFile::GetValue(const CString &keyname, const CString &valuename, float& value) const
{
	CString sValue;
	if( !GetValue(keyname,valuename,sValue) )
		return false;
	sscanf( sValue.c_str(), "%f", &value );
	return true;
}

// gets value of [keyname] valuename = 
bool IniFile::GetValue(const CString &keyname, const CString &valuename, bool& value) const
{
	CString sValue;
	if( !GetValue(keyname,valuename,sValue) )
		return false;
	value = atoi(sValue) != 0;
	return true;
}

// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
bool IniFile::SetValue(const CString &keyname, const CString &valuename, const CString &value, bool create)
{
	if (!create && keys.find(keyname) == keys.end()) //if key doesn't exist
		return false; // stop entering this key

	// find value
	if (!create && keys[keyname].find(valuename) == keys[keyname].end())
		return false;

	keys[keyname][valuename] = value;
	return true;
}

// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
bool IniFile::SetValue(const CString &keyname, const CString &valuename, int value, bool create)
{
	return SetValue(keyname, valuename, ssprintf("%d",value), create);
}

bool IniFile::SetValue(const CString &keyname, const CString &valuename, unsigned value, bool create)
{
	return SetValue(keyname, valuename, ssprintf("%u",value), create);
}

bool IniFile::SetValue(const CString &keyname, const CString &valuename, float value, bool create)
{
	return SetValue(keyname, valuename, ssprintf("%f",value), create);
}

bool IniFile::SetValue(const CString &keyname, const CString &valuename, bool value, bool create)
{
	return SetValue(keyname, valuename, ssprintf("%d",value), create);
}

// deletes specified value
// returns true if value existed and deleted, false otherwise
bool IniFile::DeleteValue(const CString &keyname, const CString &valuename)
{
	keymap::iterator k = keys.find(keyname);
	if (k == keys.end())
		return false;

	key::iterator i = k->second.find(valuename);
	if(i == k->second.end())
		return false;

	k->second.erase(i);
	return true;
}

// deletes specified key and all values contained within
// returns true if key existed and deleted, false otherwise
bool IniFile::DeleteKey(const CString &keyname)
{
	keymap::iterator k = keys.find(keyname);
	if (k == keys.end())
		return false;

	keys.erase(k);
	return true;
}

const IniFile::key *IniFile::GetKey(const CString &keyname) const
{
	keymap::const_iterator i = keys.find(keyname);
	if(i == keys.end()) return NULL;
	return &i->second;
}

void IniFile::SetValue(const CString &keyname, const key &key)
{
	keys[keyname]=key;
}

void IniFile::RenameKey(const CString &from, const CString &to)
{
	if(keys.find(from) == keys.end())
		return;
	if(keys.find(to) != keys.end())
		return;

	keys[to] = keys[from];
	keys.erase(from);
}
