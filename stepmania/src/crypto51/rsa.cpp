// rsa.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "rsa.h"
#include "asn.h"
#include "oids.h"
#include "modarith.h"
#include "nbtheory.h"
#include "sha.h"
#include "algparam.h"
#include "fips140.h"

#ifndef NDEBUG
#include "pssr.h"
#endif

#include "oaep.cpp"

NAMESPACE_BEGIN(CryptoPP)

#ifndef NDEBUG
void RSA_TestInstantiations()
{
	RSASS<PKCS1v15, SHA>::Verifier x1(1, 1);
	RSASS<PKCS1v15, SHA>::Signer x2(NullRNG(), 1);
	RSASS<PKCS1v15, SHA>::Verifier x3(x2);
	RSASS<PKCS1v15, SHA>::Verifier x4(x2.GetKey());
	RSASS<PSS, SHA>::Verifier x5(x3);
#ifndef __MWERKS__
	RSASS<PSSR, SHA>::Signer x6 = x2;
	x3 = x2;
	x6 = x2;
#endif
	RSAES<PKCS1v15>::Encryptor x7(x2);
#ifndef __GNUC__
	RSAES<PKCS1v15>::Encryptor x8(x3);
#endif
	RSAES<OAEP<SHA> >::Encryptor x9(x2);

	x4 = x2.GetKey();
}
#endif

template class OAEP<SHA>;

OID RSAFunction::GetAlgorithmID() const
{
	return ASN1::rsaEncryption();
}

void RSAFunction::BERDecodeKey(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
		m_n.BERDecode(seq);
		m_e.BERDecode(seq);
	seq.MessageEnd();
}

void RSAFunction::DEREncodeKey(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
		m_n.DEREncode(seq);
		m_e.DEREncode(seq);
	seq.MessageEnd();
}

Integer RSAFunction::ApplyFunction(const Integer &x) const
{
	DoQuickSanityCheck();
	return a_exp_b_mod_c(x, m_e, m_n);
}

bool RSAFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n.IsOdd();
	pass = pass && m_e > Integer::One() && m_e.IsOdd() && m_e < m_n;
	return pass;
}

bool RSAFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(PublicExponent)
		;
}

void RSAFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(PublicExponent)
		;
}

// *****************************************************************************

class RSAPrimeSelector : public PrimeSelector
{
public:
	RSAPrimeSelector(const Integer &e) : m_e(e) {}
	bool IsAcceptable(const Integer &candidate) const {return RelativelyPrime(m_e, candidate-Integer::One());}
	Integer m_e;
};

void InvertibleRSAFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	int modulusSize = 2048;
	alg.GetIntValue("ModulusSize", modulusSize) || alg.GetIntValue("KeySize", modulusSize);

	if (modulusSize < 16)
		throw InvalidArgument("InvertibleRSAFunction: specified modulus size is too small");

	m_e = alg.GetValueWithDefault("PublicExponent", Integer(17));

	if (m_e < 3 || m_e.IsEven())
		throw InvalidArgument("InvertibleRSAFunction: invalid public exponent");

	RSAPrimeSelector selector(m_e);
	const NameValuePairs &primeParam = MakeParametersForTwoPrimesOfEqualSize(modulusSize)
		("PointerToPrimeSelector", selector.GetSelectorPointer());
	m_p.GenerateRandom(rng, primeParam);
	m_q.GenerateRandom(rng, primeParam);

	m_d = EuclideanMultiplicativeInverse(m_e, LCM(m_p-1, m_q-1));
	assert(m_d.IsPositive());

	m_dp = m_d % (m_p-1);
	m_dq = m_d % (m_q-1);
	m_n = m_p * m_q;
	m_u = m_q.InverseMod(m_p);

	if (FIPS_140_2_ComplianceEnabled())
	{
		RSASS<PKCS1v15, SHA>::Signer signer(*this);
		RSASS<PKCS1v15, SHA>::Verifier verifier(signer);
		SignaturePairwiseConsistencyTest_FIPS_140_Only(signer, verifier);

		RSAES<OAEP<SHA> >::Decryptor decryptor(*this);
		RSAES<OAEP<SHA> >::Encryptor encryptor(decryptor);
		EncryptionPairwiseConsistencyTest_FIPS_140_Only(encryptor, decryptor);
	}
}

void InvertibleRSAFunction::Initialize(RandomNumberGenerator &rng, unsigned int keybits, const Integer &e)
{
	GenerateRandom(rng, MakeParameters("ModulusSize", (int)keybits)("PublicExponent", e+e.IsEven()));
}

void InvertibleRSAFunction::Initialize(const Integer &n, const Integer &e, const Integer &d)
{
	m_n = n;
	m_e = e;
	m_d = d;

	Integer r = --(d*e);
	while (r.IsEven())
		r >>= 1;

	ModularArithmetic modn(n);
	for (Integer i = 2; ; ++i)
	{
		Integer a = modn.Exponentiate(i, r);
		if (a == 1)
			continue;
		Integer b;
		while (a != -1)
		{
			b = modn.Square(a);
			if (b == 1)
			{
				m_p = GCD(a-1, n);
				m_q = n/m_p;
				m_dp = m_d % (m_p-1);
				m_dq = m_d % (m_q-1);
				m_u = m_q.InverseMod(m_p);
				return;
			}
			a = b;
		}
	}
}

void InvertibleRSAFunction::BERDecodeKey(BufferedTransformation &bt)
{
	BERSequenceDecoder privateKey(bt);
		word32 version;
		BERDecodeUnsigned<word32>(privateKey, version, INTEGER, 0, 0);	// check version
		m_n.BERDecode(privateKey);
		m_e.BERDecode(privateKey);
		m_d.BERDecode(privateKey);
		m_p.BERDecode(privateKey);
		m_q.BERDecode(privateKey);
		m_dp.BERDecode(privateKey);
		m_dq.BERDecode(privateKey);
		m_u.BERDecode(privateKey);
	privateKey.MessageEnd();
}

void InvertibleRSAFunction::DEREncodeKey(BufferedTransformation &bt) const
{
	DERSequenceEncoder privateKey(bt);
		DEREncodeUnsigned<word32>(privateKey, 0);	// version
		m_n.DEREncode(privateKey);
		m_e.DEREncode(privateKey);
		m_d.DEREncode(privateKey);
		m_p.DEREncode(privateKey);
		m_q.DEREncode(privateKey);
		m_dp.DEREncode(privateKey);
		m_dq.DEREncode(privateKey);
		m_u.DEREncode(privateKey);
	privateKey.MessageEnd();
}

Integer InvertibleRSAFunction::CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const 
{
	DoQuickSanityCheck();
	ModularArithmetic modn(m_n);
	Integer r(rng, Integer::One(), m_n - Integer::One());
	Integer re = modn.Exponentiate(r, m_e);
	re = modn.Multiply(re, x);			// blind
	// here we follow the notation of PKCS #1 and let u=q inverse mod p
	// but in ModRoot, u=p inverse mod q, so we reverse the order of p and q
	Integer y = ModularRoot(re, m_dq, m_dp, m_q, m_p, m_u);
	y = modn.Divide(y, r);				// unblind
	if (modn.Exponentiate(y, m_e) != x)		// check
		throw Exception(Exception::OTHER_ERROR, "InvertibleRSAFunction: computational error during private key operation");
	return y;
}

bool InvertibleRSAFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = RSAFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p.IsOdd() && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q.IsOdd() && m_q < m_n;
	pass = pass && m_d > Integer::One() && m_d.IsOdd() && m_d < m_n;
	pass = pass && m_dp > Integer::One() && m_dp.IsOdd() && m_dp < m_p;
	pass = pass && m_dq > Integer::One() && m_dq.IsOdd() && m_dq < m_q;
	pass = pass && m_u.IsPositive() && m_u < m_p;
	if (level >= 1)
	{
		pass = pass && m_p * m_q == m_n;
		pass = pass && m_e*m_d % LCM(m_p-1, m_q-1) == 1;
		pass = pass && m_dp == m_d%(m_p-1) && m_dq == m_d%(m_q-1);
		pass = pass && m_u * m_q % m_p == 1;
	}
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleRSAFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<RSAFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_GET_FUNCTION_ENTRY(PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(ModPrime1PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(ModPrime2PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

void InvertibleRSAFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<RSAFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_SET_FUNCTION_ENTRY(PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(ModPrime1PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(ModPrime2PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

NAMESPACE_END
