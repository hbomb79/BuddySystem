#include <gmp.h>
#include <gmpxx.h>
#include <cstring>

namespace rsa
{
	struct rsa_key
	{
		mpz_class p;
		mpz_class q;
		mpz_class e;
		mpz_class d;
		mpz_class z;
		mpz_class n;
	};

	struct rsa_public_key
	{
		mpz_class n;
		mpz_class e;
	};

	mpz_class generateRandomPrime();
	rsa_key generateKeyPair();
	mpz_class euclidian_gcd(mpz_class a, mpz_class b);
	mpz_class extended_euclidian(mpz_class u, mpz_class v);

	mpz_class random_bits(int n_bits);

	mpz_class cipher_byte(mpz_class byte, mpz_class n, mpz_class key);
	std::string encrypt_string(std::string data, mpz_class n, mpz_class encryption_key, mpz_class &nonce);
	std::string decrypt_string(std::string cipher, mpz_class n, mpz_class decryption_key, mpz_class &nonce);

	void initialise();
}