#ifndef CRYPTOPP_MARS_H
#define CRYPTOPP_MARS_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct MARS_Info : public FixedBlockSize<16>, public VariableKeyLength<16, 16, 56, 4>
{
	static const char *StaticAlgorithmName() {return "MARS";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#MARS">MARS</a>
class MARS : public MARS_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<MARS_Info>
	{
	public:
		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length);

	protected:
		static const word32 Sbox[512];

		FixedSizeSecBlock<word32, 40> EK;
	};

	class Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherTemplate<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherTemplate<DECRYPTION, Dec> Decryption;
};

typedef MARS::Encryption MARSEncryption;
typedef MARS::Decryption MARSDecryption;

NAMESPACE_END

#endif
