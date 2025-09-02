#----------------------------------------------------------------
#
#  4190.308 Computer Architecture (Fall 2021)
#
#  Project #3: Image Convolution in RISC-V Assembly
#
#  October 25, 2021
#
#  Jaehoon Shim (mattjs@snu.ac.kr)
#  Ikjoon Son (ikjoon.son@snu.ac.kr)
#  Seongyeop Jeong (seongyeop.jeong@snu.ac.kr)
#  Systems Software & Architecture Laboratory
#  Dept. of Computer Science and Engineering
#  Seoul National University
#
#----------------------------------------------------------------

####################
# void bmpconv(unsigned char *imgptr, int h, int w, unsigned char *k, unsigned char *outptr)
####################

	.globl bmpconv
bmpconv:
	# a0 = imgptr
	# a1 = height
	# a2 = width
	# a3 = kptr
	# a4 = outptr
	
	addi	sp, sp, -8

	# 4(sp) = valid bit width
	addi	t0, a2, -2
	slli	t1, t0, 1
	add		t1, t1, t0
	sw		t1, 4(sp)
	
	# 0(sp) = output bit height
	addi	t0, a1, -2
	sw		t0, 0(sp)

	# a1 = kernal values (2 bits each)
	# t3 = loop index
	xor		a1, a1, a1
	addi	t0, x0, 9
	addi	t3, x0, 0

LOOP_KERNAL:	
	andi	t1, a3, -4
	andi	t2, a3, 3
	lw		t1, 0(t1)
	slli	t2, t2, 3
	srl		t1, t1, t2
	andi	t1, t1, 3
	slli	t3, t3, 1
	sll		t1, t1, t3
	srli	t3, t3, 1
	or		a1, a1, t1

	addi	a3, a3, 1
	addi	t3, t3, 1
	blt		t3, t0, LOOP_KERNAL

	# a2 = input bit width (4n)
	slli	t0, a2, 1
	add		a2, a2, t0
	andi	t0, a2, 3
	xori	t0, t0, -1
	addi	t0, t0, 1
	andi	t0, t0, 3
	add		a2, a2, t0

	# leftover check
	lw		t0, 4(sp)
	andi	t0, t0, 3
	beq		t0, x0, GOOD_MAIN

BAD_MAIN:
	# t0 = outer index
	addi	t0, x0, 0
OUT_B:
	# t1 = inner index
	addi	t1, x0, 0
IN_B:
	# save registers
	addi	sp, sp, -20
	sw		ra,	16(sp)
	sw		a1, 12(sp)
	sw		a4, 8(sp)
	sw		t0, 4(sp)
	sw		t1, 0(sp)

	# function call
	# output in a3
	jal		ra, calc

	# load saved registers
	lw		t1, 0(sp)
	lw		t0, 4(sp)
	lw		a4, 8(sp)
	lw		a1, 12(sp)
	lw		ra, 16(sp)
	addi	sp, sp, 20

	# push the output, assume that a4 is always multiple of 4
	sw		a3, 0(a4)

	# update indices
	lw		t2, 4(sp)		# t2 = valid width
	addi	a0, a0, 4		# imgptr
	addi	t1, t1, 4		# inner index
	addi	a4, a4, 4		# outptr
	addi	t3, t1, 4
	bge		t2, t3, IN_B

	# calcuate leftovers
	andi	t2, t2, 3		# num. of leftovers
	add		a0, a0, t2		# calculate in inverse order
	xor		a3, a3, a3		# result will be accumulated in a3, rest will be padded to zero

LEFT:
	addi	a0, a0, -1		# imgptr
	addi	t2, t2, -1		# loop index

	addi	sp, sp, -20
	sw		ra, 16(sp)
	sw		a1, 12(sp)
	sw		a3, 8(sp)
	sw		t0,	4(sp)
	sw		t1, 0(sp)
	jal		ra, calc_single	# output in a3
	addi	t3, a3, 0		# copy output to t3
	lw		t1, 0(sp)
	lw		t0, 4(sp)
	lw		a3, 8(sp)
	lw		a1, 12(sp)
	lw		ra, 16(sp)
	addi	sp, sp, 20
	slli	a3, a3, 8
	or		a3, a3, t3
	blt		x0, t2, LEFT

	# put leftovers
	sw		a3, 0(a4)

	# move to the next row
	lw		t2, 0(sp)		# t2 = output height
	sub		t3, a2, t1		# t3 = input width - inner index
	add		a0, a0, t3		# imgptr
	addi	a4, a4, 4		# outptr
	addi	t0, t0, 1		# outer index
	blt		t0, t2, OUT_B

	addi	sp, sp, 8
	ret

#######################################################################################################################################

GOOD_MAIN:
	# t0 = outer index
	addi	t0, x0, 0
OUT_G:
	# t1 = inner index
	addi	t1, x0, 0
IN_G:
	# save registers
	addi	sp, sp, -20
	sw		ra,	16(sp)
	sw		a1, 12(sp)
	sw		a4, 8(sp)
	sw		t0, 4(sp)
	sw		t1, 0(sp)

	# function call
	# output in a3
	jal		ra, calc

	# load saved registers
	lw		t1, 0(sp)
	lw		t0, 4(sp)
	lw		a4, 8(sp)
	lw		a1, 12(sp)
	lw		ra, 16(sp)
	addi	sp, sp, 20

	# push the output, assume that a4 is always multiple of 4
	sw		a3, 0(a4)

	# update indices
	lw		t2, 4(sp)		# t2 = valid width
	addi	a0, a0, 4		# imgptr
	addi	t1, t1, 4		# inner index
	addi	a4, a4, 4		# outptr
	addi	t3, t1, 4
	bge		t2, t3, IN_G

	# move to the next row
	lw		t2, 0(sp)		# t2 = output height
	sub		t3, a2, t1		# t3 = input width - inner index
	add		a0, a0, t3		# imgptr
	addi	t0, t0, 1		# outer index
	blt		t0, t2, OUT_G

	addi	sp, sp, 8
	ret

calc:
	# a0 = imgptr (will be restored)
	# a1 = kernal values (saved)
	# a2 = input bit width (not touched)
	# a3 = result 1, 2 (don't care)
	# a4 = result 3, 4 (outptr saved)

	# 13 bits per each output result. Will be saturated to 8 bits
	# saturated 32-bit result will be stored in a3

	xor		a3, a3, a3
	xor		a4, a4, a4
	addi	t0, x0, 1		# t0 = 1 for beq
	addi	t4, x0, 3		# t4 = loop index

LOOP_CALC:
	# kernal[][0]
	lw		t2, 0(a0)
	andi	t1, a1, 3
	beq		t1, x0, L2		# if kernal is 0
	beq		t1, t0, L1		# if kernal is 1

	# (else) if kernal is -1
	# result 1
	addi	t1, a3, 0
	andi	t3, t2, 255		# always positive; no need to sign extend
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or		a3, a3, t1

	# result 2
	srli	t3, t2, 8
	andi	t3, t3, 255
	slli	t3, t3, 13
	sub		a3, a3, t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	# result 3
	addi	t1, a4, 0
	srli	t3, t2, 16
	andi	t3, t3, 255
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 24
	slli	t3, t3, 13
	sub		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6

	jal		x0, L2
L1:
	# result 1
	addi	t1, a3, 0
	andi	t3, t2, 255		# always positive; no need to sign extend
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or		a3, a3, t1

	# result 2
	srli	t3, t2, 8
	andi	t3, t3, 255
	slli	t3, t3, 13
	add		a3, a3, t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	# result 3
	addi	t1, a4, 0
	srli	t3, t2, 16
	andi	t3, t3, 255
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 24
	slli	t3, t3, 13
	add		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6
L2:

	# kernal[][1]
	srli	t1, a1, 2
	andi	t1, t1, 3
	beq		t1, x0, L4		# if kernal is 0
	beq		t1, t0, L3		# if kernal is 1

	# (else) if kernal is -1
	# result 1
	addi	t1, a3, 0
	srli	t3, t2, 24
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or 		a3, a3, t1

	lw		t2, 4(a0)
	
	# result 2
	andi	t3, t2, 255
	slli	t3, t3, 13
	sub		a3, a3, t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	# result 3
	addi	t1, a4, 0
	srli	t3, t2, 8
	andi	t3, t3, 255
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 16
	andi	t3, t3, 255
	slli	t3, t3, 13
	sub		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6
	
	jal		x0, L4
L3:	
	# result 1
	addi	t1, a3, 0
	srli	t3, t2, 24
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or 		a3, a3, t1

	lw		t2, 4(a0)
	
	# result 2
	andi	t3, t2, 255
	slli	t3, t3, 13
	add		a3, a3, t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	# result 3
	addi	t1, a4, 0
	srli	t3, t2, 8
	andi	t3, t3, 255
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 16
	andi	t3, t3, 255
	slli	t3, t3, 13
	add		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6
L4:
	lw		t2, 4(a0)

	# kernal[][2]
	srli	t1, a1, 4
	andi	t1, t1, 3
	beq		t1, x0, L6		# if kernal is 0
	beq		t1, t0, L5		# if kernal is 1

	# (else) if kernal is -1
	# result 1
	addi	t1, a3, 0
	srli	t3, t2, 16
	andi	t3, t3, 255
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or		a3, a3, t1

	# result 2
	srli	t3, t2, 24
	slli	t3, t3, 13
	sub		a3, a3,	t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	lw		t2, 8(a0)

	# result 3
	addi	t1, a4, 0
	andi	t3, t2, 255
	sub		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 8
	andi	t3, t3, 255
	slli	t3, t3, 13
	sub		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6

	jal 	x0, L6
L5:
	# result 1
	addi	t1, a3, 0
	srli	t3, t2, 16
	andi	t3, t3, 255
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a3, a3, 13
	srli	t1, t1, 19
	slli	a3, a3, 13
	or		a3, a3, t1

	# result 2
	srli	t3, t2, 24
	slli	t3, t3, 13
	add		a3, a3,	t3
	slli	a3, a3, 6
	srli	a3, a3, 6

	lw		t2, 8(a0)

	# result 3
	addi	t1, a4, 0
	andi	t3, t2, 255
	add		t1, t1, t3
	slli	t1, t1, 19
	srli	a4, a4, 13
	srli	t1, t1, 19
	slli	a4, a4, 13
	or		a4, a4, t1

	# result 4
	srli	t3, t2, 8
	andi	t3, t3, 255
	slli	t3, t3, 13
	add		a4, a4, t3
	slli	a4, a4, 6
	srli	a4, a4, 6
L6:

	addi	t4, t4, -1
	add		a0, a0, a2
	srli	a1, a1, 6
	blt		x0, t4, LOOP_CALC

	# saturation
	xor		t4, t4, t4		# t4 = temporary result
	addi	t1, x0, 255		# t1 = 255

	# t0 = result 1
	slli	t0, a3, 19
	srai	t0, t0, 19
	blt		t0, x0, L8		# less than 0
	blt		t1, t0, L7		# greater than 255

	# (else) 0 ~ 255
	or		t4, t4, t0
	jal		x0, L8
L7:
	or		t4, t4, t1
L8:

	# t0 = result 2
	slli	t0, a3, 6
	srai	t0, t0, 19
	blt		t0, x0, L10
	blt		t1, t0, L9

	# (else) 0 ~ 255
	slli	t0, t0, 8
	or		t4, t4, t0
	jal		x0, L10
L9:
	slli	t1, t1, 8
	or		t4, t4, t1
	srli	t1, t1, 8
L10:

	# t0 = result 3
	slli	t0, a4, 19
	srai	t0, t0, 19
	blt		t0, x0, L12
	blt		t1, t0, L11

	# (else) 0 ~ 255
	slli	t0, t0, 16
	or		t4, t4, t0
	jal		x0, L12
L11:
	slli	t1, t1, 16
	or		t4, t4, t1
	srli	t1, t1, 16
L12:

	# t0 = result 4
	slli	t0, a4, 6
	srai	t0, t0, 19
	blt		t0, x0, L14
	blt		t1, t0, L13

	# (else) 0 ~ 255
	slli	t0, t0, 24
	or		t4, t4, t0
	jal		x0, L14
L13:
	slli	t1, t1, 24
	or		t4, t4, t1
L14:
	addi	a3, t4, 0

	# restore a0
	sub		a0, a0, a2
	sub		a0, a0, a2
	sub		a0, a0, a2
	ret

calc_single:
	# a0 = imgptr (will be restored)
	# a1 = kernal values (saved)
	# a2 = input bit width (not touched)
	# a3 = result (saved)
	# a4 = outptr (not touched)
	# t0 = outer index (saved)
	# t1 = inner index (saved)
	# t2 = LEFT index (not touched)

	xor		a3, a3, a3
	
	# row 0
	addi	t4, x0, 3		# t4 = index
L15:
	andi	t1, a1, 3		# t1 = kurnel
	beq		t1, x0, L17		# kernal = 0
	slli	t1, t1, 30
	srai	t1, t1, 30
	andi	t0, a0, -4
	andi 	t3, a0, 3
	lw		t0, 0(t0)
	slli	t3, t3, 3
	srl		t0, t0, t3
	andi	t0, t0, 255		# t0 = pixel value
	blt		t1, x0,	L16		# kernal = -1
	# (else) kernal = 1
	add		a3, a3, t0
	jal		x0, L17
L16:
	sub		a3, a3, t0
L17:
	addi	t4, t4, -1
	addi	a0, a0, 3
	srli	a1, a1, 2
	blt		x0, t4, L15

	# row 1
	addi	a0, a0, -9
	add		a0, a0, a2
	addi	t4, x0, 3
L18:
	andi	t1, a1, 3		# t1 = kurnel
	beq		t1, x0, L20		# kernal = 0
	slli	t1, t1, 30
	srai	t1, t1, 30
	andi	t0, a0, -4
	andi 	t3, a0, 3
	lw		t0, 0(t0)
	slli	t3, t3, 3
	srl		t0, t0, t3
	andi	t0, t0, 255		# t0 = pixel value
	blt		t1, x0,	L19		# kernal = -1
	# (else) kernal = 1
	add		a3, a3, t0
	jal		x0, L20
L19:
	sub		a3, a3, t0
L20:
	addi	t4, t4, -1
	addi	a0, a0, 3
	srli	a1, a1, 2
	blt		x0, t4, L18

	# row 2
	addi	a0, a0, -9
	add		a0, a0, a2
	addi	t4, x0, 3
L21:
	andi	t1, a1, 3		# t1 = kurnel
	beq		t1, x0, L23		# kernal = 0
	slli	t1, t1, 30
	srai	t1, t1, 30
	andi	t0, a0, -4
	andi 	t3, a0, 3
	lw		t0, 0(t0)
	slli	t3, t3, 3
	srl		t0, t0, t3
	andi	t0, t0, 255		# t0 = pixel value
	blt		t1, x0,	L22		# kernal = -1
	# (else) kernal = 1
	add		a3, a3, t0
	jal		x0, L23
L22:
	sub		a3, a3, t0
L23:
	addi	t4, t4, -1
	addi	a0, a0, 3
	srli	a1, a1, 2
	blt		x0, t4, L21

	# a0 restoration
	addi	a0, a0, -9
	sub		a0, a0, a2
	sub		a0, a0, a2

	# saturation
	bge		a3, x0, L24
	addi	a3, x0, 0
	ret
L24:
	addi	t4, x0, 255
	bge		t4, a3,	L25
	addi	a3, t4, 0
	ret
L25:
	ret