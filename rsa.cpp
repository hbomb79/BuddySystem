/**
 * RSA Implementation module for 159342, Assignment 2.
 * 
 * Harry Felton - Massey ID 18032692
 */

#include <stdlib.h>
#include <iostream>
#include <gmp.h>
#include <gmpxx.h>
#include <ctime>
#include <sstream>
#include "rsa.hpp"

#define PRIME_BIT_SIZE 128
#define PRIME_TEST_RUNS 50
#define RSA_E 65537

using namespace rsa;

gmp_randclass rr(gmp_randinit_default);

/**
 * Initialises the random number class from GMP for use later. Seeds with the current system
 * time (not secure but works as a proof of concept)
 */
void rsa::initialise()
{
	rr.seed(time(NULL));
}

/**
 * Generates a random number containing n bits
 */
mpz_class rsa::random_bits(int n_bits)
{
	return rr.get_z_bits(n_bits);
}

/**
 * Returns a random number that is prime of the size PRIME_BIT_SIZE
 */
mpz_class rsa::generateRandomPrime()
{
	mpz_class ran = rsa::random_bits(PRIME_BIT_SIZE);

	// If random number is even, make it odd as all prime numbers after 2 are odd numbers!
	if (ran % 2 == 0)
		ran++;

	// Check if this number is *probably* a prime. Similar to how OpenSSL generates large random primes
	// If this number is NOT a prime, add two and try again.
	// mpz_probab_prime_p will return 0 if the number is not prime, 1 if the number is probably a prime, and 2 if it's definetely a prime.
	do
	{
		ran += 2;
	} while (mpz_probab_prime_p(ran.get_mpz_t(), PRIME_TEST_RUNS) < 1);

	return ran;
}

/**
 * Calculate the GCD of the two provided big numbers using the Euclidian algorithm
 */
mpz_class rsa::euclidian_gcd(mpz_class a, mpz_class b)
{
	// Edge cases
	if (b == 0)
		return a;
	if (a < 0 || b < 0)
	{
		throw "Calculating GCD using euclidian algorithm requires that the inputs (a,b) are both > 0";
	}

	// Algorithm states that a >= b, if a < b then swap the numbers round
	if (a < b)
	{
		mpz_class swap = a;
		a = b;
		b = swap;
	}

	// Begin euclidian GCD algorithm
	mpz_class remainder;
	while (b > 0)
	{
		remainder = a % b;
		a = b;
		b = remainder;
	}

	return a;
}

/**
 * Implementation of the extended euclidian algorithm, discarding the x/y components (i.e. only
 * solving for d). Essentially this method operates as a tweaked version of the extended 
 * euclidian algorithm, functioning as a way to find the modular inverse.
 */
mpz_class rsa::extended_euclidian(mpz_class u, mpz_class v)
{
	mpz_class inv, u1, u3, v1, v3, t1, t3, q;
	mpz_class iter;
	u1 = 1;
	u3 = u;
	v1 = 0;
	v3 = v;
	
	iter = 1;
	while (v3 != 0)
	{
		q = u3 / v3;
		t3 = u3 % v3;
		t1 = u1 + q * v1;
		u1 = v1;
		v1 = t1;
		u3 = v3;
		v3 = t3;
		iter = -iter;
	}
	if (u3 != 1)
		return 0;
	if (iter < 0)
		inv = v - u1;
	else
		inv = u1;
	return inv;
}

/**
 * Generates a key-pair for RSA based encryption. The returned rsa_key struct contains every component
 * of the key, including secret components. Only n and e should be shared as part of the public key.
 */
rsa_key rsa::generateKeyPair()
{
	rsa_key key;
	key.e = RSA_E;

	// Generate our two large primes (128 bits), ensuring they're coprime with e and not the same prime number (very unlikley)
	do
	{
		key.p = generateRandomPrime();
	} while (euclidian_gcd(key.p - 1, key.e) != 1);

	do
	{
		key.q = generateRandomPrime();
	} while (euclidian_gcd(key.q - 1, key.e) != 1 || key.p == key.q);

	// Take the product of these to calculate 'n', which is released as part of the public key.
	// 256 bit RSA encryption
	key.n = key.p * key.q;

	// Calculate phi(n) (seen as L or z)
	key.z = (key.p - 1) * (key.q - 1);

	// Calculate d using the extended euclidian algorithm/modular inverse
	// Private key component
	key.d = extended_euclidian(key.e, key.z);

	return key;
}

/**
 * Given a string of characters, encrypts *each* character as it's ASCII representation,
 * and returns a cyper-text space-delimited string. This method accepts a reference to the
 * nonce in use for this instance of the RSA communication. The nonce is applied
 * to each character (XOR), and is changed for each block encrypted. No additional work
 * is required to keep the nonce in sync with the rest of the application.
 * 
 * Should be provided to rsa::decrypt to deciper provided string after socket transportation.
 */
std::string rsa::encrypt_string(std::string data, mpz_class n, mpz_class encryption_key, mpz_class &nonce)
{
	std::string out;
	char character;
	mpz_class ascii_character;
	mpz_class cipher;

	for (std::string::iterator it = data.begin(); it != data.end(); it++)
	{
		// Fetch the character being pointed to by our iterator.
		character = *it;

		// Convert to ASCII
		ascii_character = (int)character;

		// XOR with nonce
		ascii_character = ascii_character ^ nonce;

		// Encrypt character ASCII code with n and encryption_key (could be e, or d)
		cipher = rsa::cipher_byte(ascii_character, n, encryption_key);

		// Nonce is now set to last ciper block
		nonce = cipher;

		// Append the ciper text, followed by whitespace (delimiter used when decrypting)
		out.append(cipher.get_str().append(" "));
	}

	return out;
}

/**
 * Given a string of encrypted ciper-text (" " delimited), decrypts each block and converts the
 * output to a character (expected ASCII characters as output of deciphering).
 * 
 * This method accepts a reference to the nonce in use for this instance of the RSA communication.
 * The nonce is applied to each character (XOR), and is changed for each block decrypted.
 * No additional work is required to keep the nonce in sync with the rest of the application.
 */
std::string rsa::decrypt_string(std::string data, mpz_class n, mpz_class decryption_key, mpz_class &nonce)
{
	std::string out;
	mpz_class ascii_char;
	mpz_class cipher_block;

	// Split string on each whitespace and read every token until no more are found.
	std::istringstream stream(data);
	std::string tk;
	while (stream >> tk)
	{
		// Read the token (string) in to a number
		cipher_block = tk;

		// Decipher character
		ascii_char = rsa::cipher_byte(cipher_block, n, decryption_key);

		// XOR with nonce
		ascii_char = ascii_char ^ nonce;

		// Set nonce to cipher block
		nonce = cipher_block;

		// Convert to ASCII
		out += (char)(ascii_char.get_si());
	}

	return out;
}

/**
 * Given a byte of information (assumedly an ASCII byte), encrypts/decrypts the data and returns it
 * 
 * Performs a y = (x^e) mod n calculation. As the % operator in C is more of a remainder operator
 * than a true modulus (like in Python), this function is required to implement the RSA encryption.
 * 
 * Adapted from lecture slides available on Massey Stream.
 */
mpz_class rsa::cipher_byte(mpz_class byte, mpz_class n, mpz_class key)
{
	mpz_class y = 1;
	while (key > 0)
	{
		if ((key % 2) == 0)
		{
			byte = (byte * byte) % n;
			key /= 2;
		}
		else
		{
			y = (byte * y) % n;
			key -= 1;
		}
	}

	return y;
}
