#ifndef CRYPTOPP_TIGER_H
#define CRYPTOPP_TIGER_H

#include "config.h"

#ifdef WORD64_AVAILABLE

#include "iterhash.h"

NAMESPACE_BEGIN(CryptoPP)

/// <a href="http://www.weidai.com/scan-mirror/md.html#Tiger">Tiger</a>
class Tiger : public IteratedHashWithStaticTransform<word64, LittleEndian, 64, Tiger>
{
public:
	enum {DIGESTSIZE = 24};
	Tiger() : IteratedHashWithStaticTransform<word64, LittleEndian, 64, Tiger>(DIGESTSIZE) {Init();}
	static void Transform(word64 *digest, const word64 *data);
	void TruncatedFinal(byte *hash, unsigned int size);
	static const char * StaticAlgorithmName() {return "Tiger";}

protected:
	void Init();

	static const word64 table[4*256];
};

NAMESPACE_END

#endif

#endif
