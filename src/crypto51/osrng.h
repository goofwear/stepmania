#ifndef CRYPTOPP_OSRNG_H
#define CRYPTOPP_OSRNG_H

#include "config.h"

#ifdef OS_RNG_AVAILABLE

#include "randpool.h"

//added
#include "cryptlib.h"

NAMESPACE_BEGIN(CryptoPP)

//! Exception class for Operating-System Random Number Generator.
class OS_RNG_Err : public Exception
{
public:
	OS_RNG_Err(const std::string &operation);
};

#ifdef NONBLOCKING_RNG_AVAILABLE

#ifdef CRYPTOPP_WIN32_AVAILABLE
class MicrosoftCryptoProvider
{
public:
	MicrosoftCryptoProvider();
	~MicrosoftCryptoProvider();
#if defined(_WIN64)
	typedef unsigned __int64 ProviderHandle;	// type HCRYPTPROV, avoid #include <windows.h>
#else
	typedef unsigned long ProviderHandle;
#endif
	ProviderHandle GetProviderHandle() const {return m_hProvider;}
private:
	ProviderHandle m_hProvider;
};
#endif

//! encapsulate CryptoAPI's CryptGenRandom or /dev/urandom
class NonblockingRng : public RandomNumberGenerator
{
public:
	NonblockingRng();
	~NonblockingRng();
	byte GenerateByte();
	void GenerateBlock(byte *output, unsigned int size);

protected:
#ifdef CRYPTOPP_WIN32_AVAILABLE
#	ifndef WORKAROUND_MS_BUG_Q258000
		MicrosoftCryptoProvider m_Provider;
#	endif
#else
	int m_fd;
#endif
};

#endif

#ifdef BLOCKING_RNG_AVAILABLE

//! encapsulate /dev/random
class BlockingRng : public RandomNumberGenerator
{
public:
	BlockingRng();
	~BlockingRng();
	byte GenerateByte();
	void GenerateBlock(byte *output, unsigned int size);

protected:
	int m_fd;
};

#endif

NAMESPACE_END

#endif

#endif
