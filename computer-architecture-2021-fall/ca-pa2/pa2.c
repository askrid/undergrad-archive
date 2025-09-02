//---------------------------------------------------------------
//
//  4190.308 Computer Architecture (Fall 2021)
//
//  Project #2: FP10 (10-bit floating point) Representation
//
//  October 5, 2021
//
//  Jaehoon Shim (mattjs@snu.ac.kr)
//  Ikjoon Son (ikjoon.son@snu.ac.kr)
//  Seongyeop Jeong (seongyeop.jeong@snu.ac.kr)
//  Systems Software & Architecture Laboratory
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//---------------------------------------------------------------

#include "pa2.h"

#define FP10_INF 0x01F0u
#define FP10_NAN 0x01FFu
#define FP32_INF 0x7F800000u
#define FP32_NAN 0x7FFFFFFFu
#define L_MASK 0x10000000u
#define R_MASK 0x08000000u
#define S_MASK 0x07FFFFFFu
#define FP32_SIGN_MASK 0x80000000u
#define FP32_EXP_MASK 0x7F800000u
#define FP32_MTS_MASK 0x007FFFFFu
#define R_MASK_DOWN 0x00040000u
#define LS_MASK_DOWN 0x000BFFFFu

/* Convert 32-bit signed integer to 10-bit floating point */
fp10 int_fp10(int n)
{
	fp10 res = 0u;

	// zero case
	if (!n) {
		return res;
	}

	unsigned sign = (n < 0) ? 0b1111111u : 0b0000000u;
	unsigned exp = 0u;
	unsigned mts = 0u;

	unsigned n_ = (sign) ? -n : n;

	// get exponent (not biased)
	while (n_ > 1) {
		n_ >>= 1;
		exp++;
	}

	// get mantissa
	n_ = (sign) ? -n : n;
	unsigned mask = (1 << exp) - 1;
	mts = (n_ & mask) << (32 - exp);

	// round to even
	if (mts & R_MASK && (mts & L_MASK || mts & S_MASK)) {
		// round up
		mts = (mts >> 28) + 1;
		if (mts & 0b10000) {
			mts = 0u;
			exp++;
		}
	} else {
		// round down
		mts >>= 28;
	}

	// apply exponent bias
	exp += 15;

	// infinity case
	if (exp >= 31) {
		return FP10_INF | (sign << 9);
	}

	// set the result
	res |= (mts);
	res |= (exp << 4);
	res |= (sign << 9);

	return res;
}

/* Convert 10-bit floating point to 32-bit signed integer */
int fp10_int(fp10 x)
{
	int res = 0;

	unsigned sign = (x >> 9) & 1;
	unsigned exp = (x >> 4) & ((1 << 5) - 1);
	unsigned mts = x & ((1 << 4) - 1);

	// zero case
	if (exp < 15) {
		return 0;
	}

	// special case
	if (exp == 31) {
		return 0x80000000;
	}

	// apply exponent bias
	exp -= 15;

	// signficand
	res = mts | (1u << 4);

	// multiply by 2^exponent
	if (exp > 4) {
		res <<= (exp - 4);
	} else {
		res >>= (4 - exp);
	}

	// apply sign bit
	res = (sign) ? -res : res;

	return res;
}

/* Convert 32-bit single-precision floating point to 10-bit floating point */
fp10 float_fp10(float f)
{
	register unsigned f_ = *((unsigned*) &f);
	register unsigned sign = (f_ & FP32_SIGN_MASK) ? 0xFFFFFE00u : 0u;
	register unsigned exp = f_ & FP32_EXP_MASK;
	register unsigned mts = f_ & FP32_MTS_MASK;

	exp >>= 23u;

	// NaN case
	if (exp == 255u && mts) {
		return FP10_NAN | sign;
	}

	// denormalized case; biased exponent < -14
	if (exp < 113u) {	
		mts >>= 113u - exp;
		mts |= 0x00400000u >> (112u - exp);
		exp = 112u;
	}

	// round to even
	if (mts & R_MASK_DOWN && mts & LS_MASK_DOWN) {
		// round up
		mts >>= 19u;
		++mts;
		exp += mts >> 4u;
		mts &= 0x1u;
	} else {
		// round down
		mts >>= 19u;
	}

	// infinity case
	if (exp > 142u) {
		return FP10_INF | sign;
	}

	// apply exponent bias; -127 + 15
	exp -= 112u;
	exp <<= 4u;

	return sign | exp | mts;
}

/* Convert 10-bit floating point to 32-bit single-precision floating point */
float fp10_float(fp10 x)
{
	unsigned res = 0.0f;

	unsigned sign = (x >> 9) & 1;
	unsigned exp = (x >> 4) & ((1 << 5) - 1);
	unsigned mts = x & ((1 << 4) - 1);

	// special case
	if (exp == 31) {
		if (mts) {
			res = FP32_NAN | sign << 31;
			return *((float*) &res);
		} else {
			res = FP32_INF | sign << 31;
			return *((float*) &res);
		}
	}

	unsigned offset = 0;

	// denormalized case
	if (exp == 0) {
		if (mts) {
			mts <<= 28;
			while (!(mts & (1 << 31))) {
				offset++;
				mts <<= 1;
			}
			mts <<= 1;
			mts >>= 28;
		} else {
			res = 0u | sign << 31;
			return *((float*) &res);
		}
	}

	// apply exponent bias for float
	exp += 112 - offset;

	// set the result
	res |= (mts << 19);
	res |= (exp << 23);
	res |= (sign << 31);

	return *((float*) &res);
}
