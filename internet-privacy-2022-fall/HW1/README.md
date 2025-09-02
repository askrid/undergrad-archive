# Internet Privacy (2022 Fall) Programming Assignment 1

## Subject

RSA Encryption and Decryption

## Author Information

| Name | Student ID | Email | Phone |
| --- | --- | --- | --- |
| Choi Joonwoo (최준우) | 2020-17316 | joonwoo3023@gmail.com | 010-5538-9809

## Environment

- macOS 12.4 x86_64
- Darwin Kernel Version 21.5.0
- Apple clang version 13.1.6

## Libraries

- gmp.h
- iostream
- string

## Build

```
g++ rsa.cpp -o rsa -lgmp -Wall -std=c++20 -O2
```

## I/O Specification

- Every value is represented in decimal.
- Inputs are passed through command line arguments.

### Keygen

#### Input

- p and q are two distinct prime numbers.

```
./rsa -keygen <p> <q>
```

#### Output

- pvtkey is formatted as "n.p.q.d" where n = p * q and d is a private key.
- pubkey is formatted as "n.e" where n = p * q and e is a public key.
- No empty line between pvtkey and pubkey.

```
<pvtkey>
<pubkey>
```

### Encrypt

#### Input

- pubkey format is same as the output of keygen.

```
./rsa -encrypt <pubkey> <plaintext>
```

#### Output

```
<ciphertext>
```

### Decrypt

#### Input

- pvtkey format is same as the output of keygen.

```
./rsa -decrypt <pvtkey> <ciphertext>
```

#### Output

```
<plaintext>
```

## Example Usage

One can run the script by `sh ./example.sh`.

### 1024-bit Prime Numbers

- p: 95390371376281656517869309071184480318094513757928276940696889329837042175995208893791445707048541176899746407546249800895381949763736416462619134270028291794699744955661557305322922595531025019516495158905823924250842708941024027405792262069148248373146230695561521143057775443673090561029280197181224600581
- q: 168853392424265072131661702675492462897192975150313452025843096999078698798903471286571308484150387651321702346418963974574189363182385553906621456610583808399215452343870610620837784614553332920154539249352880313854845656003604339885799167141743149303189279103205505904095458723704412274796443422311017144517

### Plaintext

- 1111111111111111222222222222222233333333333333334444444444444444555555555555555566666666666666667777777777777777888888888888888899999999999999990000000000000000