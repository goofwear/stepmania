#ifndef CRYPTOPP_CRC32_H
#define CRYPTOPP_CRC32_H

#include "cryptlib.h"

NAMESPACE_BEGIN(CryptoPP)

const word32 CRC32_NEGL = 0xffffffffL;

#ifdef IS_LITTLE_ENDIAN
#define CRC32_INDEX(c) (c & 0xff)
#define CRC32_SHIFTED(c) (c >> 8)
#else
#define CRC32_INDEX(c) (c >> 24)
#define CRC32_SHIFTED(c) (c << 8)
#endif

//! CRC Checksum Calculation
class CRC32 : public HashTransformation
{
public:
	enum {DIGESTSIZE = 4};
	CRC32();
	void Update(const byte *input, unsigned int length);
	void TruncatedFinal(byte *hash, unsigned int size);
	unsigned int DigestSize() const {return DIGESTSIZE;}

	void UpdateByte(byte b) {m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc);}
	byte GetCrcByte(unsigned int i) const {return ((byte *)&(m_crc))[i];}

private:
	void Reset() {m_crc = CRC32_NEGL;}
	
	static const word32 m_tab[256];
	word32 m_crc;
};

NAMESPACE_END

#endif
