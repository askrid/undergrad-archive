#include <gmp.h>
#include <iostream>
#include <string>

enum ArgOptions
{
    KEYGEN,
    ENCRYPT,
    DECRYPT,
    INVALID
};

ArgOptions resolveArgs(int argc, char *argv[]);
void keygen(char *p_str, char *q_str);
void encrypt(char *pub_key, char *plain);
void decrypt(char *pvt_key, char *cipher);

int main(int argc, char *argv[])
{
    switch (resolveArgs(argc, argv))
    {
    case KEYGEN:
        keygen(argv[2], argv[3]);
        break;
    case ENCRYPT:
        encrypt(argv[2], argv[3]);
        break;
    case DECRYPT:
        decrypt(argv[2], argv[3]);
        break;
    case INVALID:
    default:
        std::cerr << "[ERROR] Invalid arugment: " << argv[1] << std::endl;
        return -1;
    }

    return 0;
}

/* Resolve command line argument to a corresponding enum value. */
ArgOptions resolveArgs(int argc, char *argv[])
{
    if (argc == 1)
    {
        std::cerr << "Usage: [-keygen <p> <q>] [-encrypt <pubkey> <plaintext>] [-decrypt <pvtkey> <ciphertext>]" << std::endl;
        exit(-1);
    };

    char *arg = argv[1];

    // Keygen
    if (std::strcmp(arg, "-keygen") == 0)
    {
        if (argc != 4)
        {
            std::cerr << "Usage: [-keygen <p> <q>]" << std::endl;
            exit(-1);
        }
        return KEYGEN;
    }

    // Encrypt
    if (std::strcmp(arg, "-encrypt") == 0)
    {
        if (argc != 4)
        {
            std::cerr << "Usage: [-encrypt <pubkey> <plaintext>]" << std::endl;
            exit(-1);
        }
        return ENCRYPT;
    }

    // Decrypt
    if (std::strcmp(arg, "-decrypt") == 0)
    {
        if (argc != 4)
        {
            std::cerr << "Usage: [-decrypt <pvtkey> <ciphertext>]" << std::endl;
            exit(-1);
        }
        return DECRYPT;
    }

    return INVALID;
};

/* RSA - Create a RSA key pair from two prime numbers. */
void keygen(char *p_str, char *q_str)
{
    if (strcmp(p_str, q_str) == 0)
    {
        std::cerr << "[ERROR] Provide two discrete prime numbers." << std::endl;
        exit(-1);
    }

    // Initialzie p and q.
    mpz_t p, q;
    mpz_init_set_str(p, p_str, 10);
    mpz_init_set_str(q, q_str, 10);

    // Test if p and q are prime numbers.
    if (mpz_probab_prime_p(p, 50) < 1)
    {
        std::cerr << "[ERROR] " << p_str << " is not a prime number" << std::endl;
        exit(-1);
    }

    if (mpz_probab_prime_p(q, 50) < 1)
    {
        std::cerr << "[ERROR] " << q_str << " is not a prime number" << std::endl;
        exit(-1);
    }

    // Calculate n = p * q
    mpz_t n;
    mpz_init(n);
    mpz_mul(n, p, q);

    // Calculate totient = (p - 1)(q - 1)
    mpz_t totient;
    mpz_init(totient);
    mpz_sub_ui(p, p, 1); // p--
    mpz_sub_ui(q, q, 1); // q--
    mpz_mul(totient, p, q);
    mpz_add_ui(p, p, 1); // p++
    mpz_add_ui(q, q, 1); // q++

    // Select e s.t. gcd(e, totient) = 1, starting from 3.
    u_long e_i = 3UL;
    while (mpz_gcd_ui(NULL, totient, e_i++) != 1)
        ;
    e_i--;
    mpz_t e;
    mpz_init_set_ui(e, e_i);

    // Get d s.t. ed = 1 (mod totient)
    mpz_t d;
    mpz_init(d);
    mpz_invert(d, e, totient);

    // Print private key in format n.p.q.d
    std::cout << mpz_get_str(NULL, 10, n) << "."
              << mpz_get_str(NULL, 10, p) << "."
              << mpz_get_str(NULL, 10, q) << "."
              << mpz_get_str(NULL, 10, d) << std::endl;

    // Print public key in format n.e
    std::cout << mpz_get_str(NULL, 10, n) << "."
              << mpz_get_str(NULL, 10, e) << std::endl;
}

/* RSA - Encrypt a plaintext. */
void encrypt(char *pub_key, char *plain)
{
    // Parse n and e from the public key.
    std::string pub_key_str(pub_key), n_str, e_str;
    n_str = pub_key_str.substr(0, pub_key_str.find(".")); // n
    pub_key_str.erase(0, pub_key_str.find(".") + 1);
    e_str = pub_key_str.substr(); // e

    // Initialize n and e.
    mpz_t n, e;
    mpz_init_set_str(n, n_str.c_str(), 10);
    mpz_init_set_str(e, e_str.c_str(), 10);

    // Parse and initialize plaintext m.
    mpz_t m;
    mpz_init_set_str(m, plain, 10);

    // Check if the message m is smaller than the modulus n.
    if (mpz_cmp(m, n) >= 0)
    {
        std::cerr << "[ERROR] Message should be smaller than the modulus " << mpz_get_str(NULL, 10, n) << std::endl;
        exit(-1);
    };

    // Create ciphertext c = m^e (mod n)
    mpz_t c;
    mpz_init(c);
    mpz_powm(c, m, e, n);

    // Print ciphertext.
    std::cout << mpz_get_str(NULL, 10, c) << std::endl;
}

/* RSA - Decrypt a ciphertext. */
void decrypt(char *pvt_key, char *cipher)
{
    // Parse n, p, q, and d from the private key.
    std::string pvt_key_str(pvt_key), n_str, p_str, q_str, d_str;
    n_str = pvt_key_str.substr(0, pvt_key_str.find(".")); // n
    pvt_key_str.erase(0, pvt_key_str.find(".") + 1);
    p_str = pvt_key_str.substr(0, pvt_key_str.find(".")); // p
    pvt_key_str.erase(0, pvt_key_str.find(".") + 1);
    q_str = pvt_key_str.substr(0, pvt_key_str.find(".")); // q
    pvt_key_str.erase(0, pvt_key_str.find(".") + 1);
    d_str = pvt_key_str.substr(); // d

    // Initialize n, p, q, and d.
    mpz_t n, p, q, d;
    mpz_init_set_str(n, n_str.c_str(), 10);
    mpz_init_set_str(p, p_str.c_str(), 10);
    mpz_init_set_str(q, q_str.c_str(), 10);
    mpz_init_set_str(d, d_str.c_str(), 10);

    // Parse and initialize ciphertext c.
    mpz_t c;
    mpz_init_set_str(c, cipher, 10);

    /* Start computing plaintext with CRT. */

    // Get c1 and c2 s.t. c1 = c^d (mod p) and c2 = c^d (mod q).
    mpz_t c1, c2;
    mpz_inits(c1, c2, NULL);
    mpz_powm(c1, c, d, p);
    mpz_powm(c2, c, d, q);

    // Get y1 and y2 s.t. y1 * q = 1 (mod p) and y2 * p = 1 (mod q).
    mpz_t y1, y2;
    mpz_inits(y1, y2, NULL);
    mpz_invert(y1, q, p);
    mpz_invert(y2, p, q);

    // Calculate plaintext x = x1 + x2 = c1 * y1 * q + c2 * y2 * p.
    mpz_t x, x1, x2;
    mpz_inits(x, x1, x2, NULL);
    mpz_mul(x1, c1, y1); // x1 = c1 * y1 * q
    mpz_mul(x1, x1, q);
    mpz_mul(x2, c2, y2); // x2 = c2 * y2 * p
    mpz_mul(x2, x2, p);
    mpz_add(x, x1, x2); // x = x1 + x2 (mod n)
    mpz_mod(x, x, n);

    // Print plaintext.
    std::cout << mpz_get_str(NULL, 10, x) << std::endl;
}