#include "global.h"
#include "CryptManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "RageFileManager.h"
#include "crypto/CryptMD5.h"

#include "arch/arch_platform.h" // hack: HAVE_ stuff should not be defined by arch

CryptManager*	CRYPTMAN	= NULL;	// global and accessable from anywhere in our program

static const CString PRIVATE_KEY_PATH = "Data/private.rsa";
static const CString PUBLIC_KEY_PATH = "Data/public.rsa";
static const CString ALTERNATE_PUBLIC_KEY_DIR = "Data/keys/";

#if !defined(HAVE_CRYPTOPP)
CryptManager::CryptManager() { }
CryptManager::~CryptManager() { }
void CryptManager::GenerateRSAKey( unsigned int keyLength, CString privFilename, CString pubFilename, CString seed ) { }
void CryptManager::SignFileToFile( CString sPath, CString sSignatureFile ) { }
bool CryptManager::VerifyFile( RageFileBasic &file, CString sSignature, CString sPublicKey, CString &sError ) { return true; }
bool CryptManager::VerifyFileWithFile( CString sPath, CString sSignatureFile )
{
	return true;
}

bool CryptManager::Verify( CString sPath, CString sSignature )
{
	return true;
}

#else

// crypt headers
#include "CryptHelpers.h"
#include "crypto51/sha.h"
#include "crypto51/rsa.h"
#include "crypto51/osrng.h"
#include <memory>

using namespace CryptoPP;

static const int KEY_LENGTH = 1024;
#define MAX_SIGNATURE_SIZE_BYTES 1024	// 1 KB

CryptManager::CryptManager()
{
	//
	// generate keys if none are available
	//
	if( PREFSMAN->m_bSignProfileData )
	{
		if( !DoesFileExist(PRIVATE_KEY_PATH) || !DoesFileExist(PUBLIC_KEY_PATH) )
		{
			LOG->Warn( "Keys missing.  Generating new keys" );
			GenerateRSAKey( KEY_LENGTH, PRIVATE_KEY_PATH, PUBLIC_KEY_PATH, "aoksdjaksd" );
			FlushDirCache();
		}
	}
}

CryptManager::~CryptManager()
{

}

static bool WriteFile( CString sFile, CString sBuf )
{
	RageFile output;
	if( !output.Open(sFile, RageFile::WRITE) )
	{
		LOG->Warn( "WriteFile: opening %s failed: %s", sFile.c_str(), output.GetError().c_str() );
		return false;
	}
	
	if( output.Write(sBuf) == -1 || output.Flush() == -1 )
	{
		LOG->Warn( "WriteFile: writing %s failed: %s", sFile.c_str(), output.GetError().c_str() );
		output.Close();
		FILEMAN->Remove( sFile );
		return false;
	}

	return true;
}

void CryptManager::GenerateRSAKey( unsigned int keyLength, CString privFilename, CString pubFilename, CString seed )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sPubKey, sPrivKey;
	try
	{
		NonblockingRng rng;

		RSASSA_PKCS1v15_SHA_Signer priv(rng, keyLength);
		StringSink privFile( sPrivKey );
		priv.DEREncode(privFile);
		privFile.MessageEnd();

		RSASSA_PKCS1v15_SHA_Verifier pub(priv);
		StringSink pubFile( sPubKey );
		pub.DEREncode(pubFile);
		pubFile.MessageEnd();
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "GenerateRSAKey failed: %s", s.what() );
		return;
	}

	if( !WriteFile(pubFilename, sPubKey) )
		return;

	if( !WriteFile(privFilename, sPrivKey) )
	{
		FILEMAN->Remove( privFilename );
		return;
	}
}

void CryptManager::SignFileToFile( CString sPath, CString sSignatureFile )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sPrivFilename = PRIVATE_KEY_PATH;
	CString sMessageFilename = sPath;
	if( sSignatureFile.empty() )
		sSignatureFile = sPath + SIGNATURE_APPEND;

	CString sPrivKey;
	if( !GetFileContents(sPrivFilename, sPrivKey) )
		return;

	if( !IsAFile(sMessageFilename) )
	{
		LOG->Trace( "SignFileToFile: \"%s\" doesn't exist", sMessageFilename.c_str() );
		return;
	}

	RageFile file;
	if( !file.Open(sMessageFilename) )
	{
		LOG->Warn( "VerifyFileWithFile: open(%s) failed: %s", sPath.c_str(), file.GetError().c_str() );
		return;
	}

	CString sSignature;
	CString sError;
	if( !SignFile(file, sPrivKey, sSignature, sError) )
	{
		LOG->Warn( "SignFileToFile failed: %s", sError.c_str() );
		return;
	}

	WriteFile( sSignatureFile, sSignature );
}

bool CryptManager::SignFile( RageFileBasic &file, CString sPrivKey, CString &sSignatureOut, CString &sError )
{
	try {
		StringSource privFile( sPrivKey, true );
		RSASSA_PKCS1v15_SHA_Signer priv(privFile);
		NonblockingRng rng;

		/* RageFileSource will delete the file we give to it, so make a copy. */
		RageFileSource f( file.Copy(), true, new SignerFilter(rng, priv, new StringSink(sSignatureOut)) );
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "SignFileToFile failed: %s", s.what() );
		return false;
	}

	return true;
}

bool CryptManager::VerifyFileWithFile( CString sPath, CString sSignatureFile )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	if( VerifyFileWithFile(sPath, sSignatureFile, PUBLIC_KEY_PATH) )
		return true;

	vector<CString> asKeys;
	GetDirListing( ALTERNATE_PUBLIC_KEY_DIR, asKeys, false, true );
	for( unsigned i = 0; i < asKeys.size(); ++i )
	{
		const CString &sKey = asKeys[i];
		LOG->Trace( "Trying alternate key \"%s\" ...", sKey.c_str() );

		if( VerifyFileWithFile(sPath, sSignatureFile, sKey) )
			return true;
	}

	return false;
}

bool CryptManager::VerifyFileWithFile( CString sPath, CString sSignatureFile, CString sPublicKeyFile )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sMessageFilename = sPath;
	if( sSignatureFile.empty() )
		sSignatureFile = sPath + SIGNATURE_APPEND;

	CString sPublicKey;
	if( !GetFileContents(sPublicKeyFile, sPublicKey) )
		return false;

	int iBytes = FILEMAN->GetFileSizeInBytes( sSignatureFile );
	if( iBytes > MAX_SIGNATURE_SIZE_BYTES )
		return false;

	CString sSignature;
	if( !GetFileContents(sSignatureFile, sSignature) )
		return false;

	RageFile file;
	if( !file.Open(sPath) )
	{
		LOG->Warn( "VerifyFileWithFile: open(%s) failed: %s", sPath.c_str(), file.GetError().c_str() );
		return false;
	}

	CString sError;
	if( !VerifyFile(file, sSignature, sPublicKey, sError) )
	{
		LOG->Warn( "VerifyFile failed: %s", sPath.c_str(), sError.c_str() );
		return false;
	}

	return true;
}

bool CryptManager::VerifyFile( RageFileBasic &file, CString sSignature, CString sPublicKey, CString &sError )
{
	try {
		StringSource pubFile( sPublicKey, true );
		RSASSA_PKCS1v15_SHA_Verifier pub(pubFile);

		if( sSignature.size() != pub.SignatureLength() )
			return false;

		VerifierFilter *verifierFilter = new VerifierFilter(pub);
		verifierFilter->Put( (byte *) sSignature.data(), sSignature.size() );

		/* RageFileSource will delete the file we give to it, so make a copy. */
		RageFileSource f( file.Copy(), true, verifierFilter );

		return verifierFilter->GetLastResult();
	} catch( const CryptoPP::Exception &s ) {
		sError = s.what();
		return false;
	}
}

bool CryptManager::Verify( CString sPath, CString sSignature )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sPublicKeyFile = PUBLIC_KEY_PATH;
	CString sMessageFilename = sPath;

	CString sPublicKey;
	if( !GetFileContents(sPublicKeyFile, sPublicKey) )
		return false;

	RageFile file;
	if( !file.Open(sPath) )
	{
		LOG->Warn( "Verify: open(%s) failed: %s", sPath.c_str(), file.GetError().c_str() );
		return false;
	}

	CString sError;
	if( !VerifyFile(file, sSignature, sPublicKey, sError) )
	{
		LOG->Warn( "Verify failed: %s", sPath.c_str(), sError.c_str() );
		return false;
	}

	return true;
}
#endif

static CString BinaryToHex( const unsigned char *string, int iNumBytes )
{
	CString s;
	for( int i=0; i<iNumBytes; i++ )
	{
		unsigned val = string[i];
		s += ssprintf( "%x", val );
	}
	return s;
}

CString CryptManager::GetMD5( CString fn )
{
       struct MD5Context md5c;
       unsigned char digest[16];
       int iBytesRead;
       unsigned char buffer[1024];

       RageFile file;
       if( !file.Open( fn, RageFile::READ ) )
       {
               LOG->Warn( "GetMD5: Failed to open file '%s'", fn.c_str() );
               return "";
       }

       MD5Init(&md5c);
       while( !file.AtEOF() && file.GetError().empty() )
       {
               iBytesRead = file.Read( buffer, sizeof(buffer) );
               MD5Update(&md5c, buffer, iBytesRead);
       }
       MD5Final(digest, &md5c);

       return BinaryToHex( digest, sizeof(digest) );
}

CString CryptManager::GetPublicKeyFileName()
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	return PUBLIC_KEY_PATH;
}

/*
 * (c) 2004 Chris Danford
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
