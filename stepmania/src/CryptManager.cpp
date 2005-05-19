#include "global.h"
#include "CryptManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "RageFileManager.h"
#include "crypto/CryptMD5.h"

CryptManager*	CRYPTMAN	= NULL;	// global and accessable from anywhere in our program

static const CString PRIVATE_KEY_PATH = "Data/private.rsa";
static const CString PUBLIC_KEY_PATH = "Data/public.rsa";

#if !defined(HAVE_CRYPTOPP)
CryptManager::CryptManager() { }
CryptManager::~CryptManager() { }
void CryptManager::GenerateRSAKey( unsigned int keyLength, CString privFilename, CString pubFilename, CString seed ) { }
void CryptManager::SignFileToFile( CString sPath, CString sSignatureFile ) { }
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

	if( !IsAFile(sPrivFilename) )
	{
		LOG->Trace( "SignFileToFile: \"%s\" doesn't exist", sSignatureFile.c_str() );
		return;
	}

	if( !IsAFile(sMessageFilename) )
	{
		LOG->Trace( "SignFileToFile: \"%s\" doesn't exist", sMessageFilename.c_str() );
		return;
	}

	CString sSignature;
	try {
		RageFileSource privFile(sPrivFilename, true);
		RSASSA_PKCS1v15_SHA_Signer priv(privFile);
		NonblockingRng rng;

		RageFileSource f(sMessageFilename, true, new SignerFilter(rng, priv, new StringSink(sSignature)));
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "SignFileToFile failed: %s", s.what() );
		return;
	}

	WriteFile( sSignatureFile, sSignature );
}

bool CryptManager::VerifyFileWithFile( CString sPath, CString sSignatureFile )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sPubFilename = PUBLIC_KEY_PATH;
	CString sMessageFilename = sPath;
	if( sSignatureFile.empty() )
		sSignatureFile = sPath + SIGNATURE_APPEND;

	if( !IsAFile(sPubFilename) )
		return false;

	if( !IsAFile(sSignatureFile) )
		return false;

	int iBytes = FILEMAN->GetFileSizeInBytes( sSignatureFile );
	if( iBytes > MAX_SIGNATURE_SIZE_BYTES )
		return false;

	try {
		RageFileSource pubFile(sPubFilename, true);
		RSASSA_PKCS1v15_SHA_Verifier pub(pubFile);

		RageFileSource signatureFile(sSignatureFile, true);
		if (signatureFile.MaxRetrievable() != pub.SignatureLength())
			return false;
		SecByteBlock signature(pub.SignatureLength());
		signatureFile.Get(signature, signature.size());

		VerifierFilter *verifierFilter = new VerifierFilter(pub);
		verifierFilter->Put(signature, pub.SignatureLength());
		RageFileSource f(sMessageFilename, true, verifierFilter);

		return verifierFilter->GetLastResult();
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "VerifyFileWithFile(%s,%s) failed: %s", sPath.c_str(), sSignatureFile.c_str(), s.what() );
		return false;
	}
}

bool CryptManager::Verify( CString sPath, CString sSignature )
{
	ASSERT( PREFSMAN->m_bSignProfileData );

	CString sPubFilename = PUBLIC_KEY_PATH;
	CString sMessageFilename = sPath;

	if( !IsAFile(sPubFilename) )
		return false;

	try {
		RageFileSource pubFile(sPubFilename, true);
		RSASSA_PKCS1v15_SHA_Verifier pub(pubFile);

		StringSource signatureFile(sSignature, true);
		if (signatureFile.MaxRetrievable() != pub.SignatureLength())
			return false;
		SecByteBlock signature(pub.SignatureLength());
		signatureFile.Get(signature, signature.size());

		VerifierFilter *verifierFilter = new VerifierFilter(pub);
		verifierFilter->Put(signature, pub.SignatureLength());
		RageFileSource f(sMessageFilename, true, verifierFilter);

		return verifierFilter->GetLastResult();
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "Verify(%s,sig) failed: %s", sPath.c_str(), s.what() );
		return false;
	}
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
