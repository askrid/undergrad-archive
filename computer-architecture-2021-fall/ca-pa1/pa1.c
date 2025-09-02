//---------------------------------------------------------------
//
//  4190.308 Computer Architecture (Fall 2021)
//
//  Project #1: Run-Length Encoding
//
//  September 14, 2021
//
//  Jaehoon Shim (mattjs@snu.ac.kr)
//  Ikjoon Son (ikjoon.son@snu.ac.kr)
//  Seongyeop Jeong (seongyeop.jeong@snu.ac.kr)
//  Systems Software & Architecture Laboratory
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//---------------------------------------------------------------


#define BYTE_SIZE 8 * sizeof(char)

int put(char* const dst, int* const dst_byte_idx, char* const dst_bit_idx, const char input, const int iter, const int input_len, const int dstlen, int* const dst_byte_len);

int encode(const char* const src, const int srclen, char* const dst, const int dstlen) {
  char target       = 0;    // target bit starts with 0
  char run_cnt      = 0;    // max: 7 == 0b111
  char src_byte     = 0;
  char src_bit_idx  = 0;
  char dst_bit_idx  = 0;
  int src_byte_idx  = 0;
  int dst_byte_idx  = 0;
  int dst_byte_len  = 0;    // actual length of the output (byte)

  while (src_byte_idx < srclen) {
    src_byte = *(src+src_byte_idx);

    if (run_cnt < 0b111 && (((src_byte >> (BYTE_SIZE-1-src_bit_idx)) & 0b1) == target)) {
      run_cnt++;

      // update indices for next iteration
      src_bit_idx++;
      src_byte_idx += (int) (src_bit_idx / BYTE_SIZE);
      src_bit_idx %= BYTE_SIZE;
    } else {
      if (!put(dst, &dst_byte_idx, &dst_bit_idx, run_cnt, 1, 3, dstlen, &dst_byte_len))
        return -1;
      
      target = 1 - target;
      run_cnt = 0;
    }

  }
  
  if (srclen && !put(dst, &dst_byte_idx, &dst_bit_idx, run_cnt, 1, 3, dstlen, &dst_byte_len))
    return -1;

  return dst_byte_len;
}

int decode(const char* const src, const int srclen, char* const dst, const int dstlen) {
    char target       = 0;    // target bit starts with 0
    char run_cnt      = 0;    // 3 bits from src+src_byte_idx
    char src_byte     = 0;
    char src_bit_idx  = 0;
    char dst_bit_idx  = 0;
    int src_byte_idx  = 0;
    int dst_byte_idx  = 0;
    int dst_byte_len  = 0;    // actual length of the output in bytes

    while (src_byte_idx < srclen) {
      src_byte = *(src+src_byte_idx);

      // get 3 bits from src_byte
      if (src_bit_idx <= BYTE_SIZE-3) {
        run_cnt = (src_byte >> (BYTE_SIZE-3-src_bit_idx)) & 0b111;
      } else if (src_byte_idx + 1 < srclen) {
        run_cnt = ((src_byte << (src_bit_idx-BYTE_SIZE+3)) | ((unsigned char) *(src+src_byte_idx+1) >> (BYTE_SIZE+5-src_bit_idx))) & 0b111;
      } else {
        break;
      }

      // update indices for next iteration
      src_bit_idx += 3;
      src_byte_idx += (int) (src_bit_idx / BYTE_SIZE);
      src_bit_idx %= BYTE_SIZE;

      if (!put(dst, &dst_byte_idx, &dst_bit_idx, target, (int) run_cnt, 1, dstlen, &dst_byte_len))
        return -1;

      target = 1 - target;
    }

    return dst_byte_len;
}

int put(char* const dst, int* const dst_byte_idx, char* const dst_bit_idx, const char input, const int iter, const int input_len, const int dstlen, int* const dst_byte_len) {
  int byte_idx    = *dst_byte_idx;
  char bit_idx    = *dst_bit_idx;
  char input_bit  = 0;

  // check memory space
  *dst_byte_len = (iter*input_len) ? byte_idx + 1 + (bit_idx + iter*input_len > BYTE_SIZE) : *dst_byte_len;
  if (*dst_byte_len > dstlen)
    return 0;

  // put (iter*input_len) bits from input to dst+byte_idx
  for (int i = 0; i < iter; i++) {
    for (int j = 0; j < input_len; j++) {
      // get a bit from the input
      input_bit = (input >> (input_len-1 - j)) & 0b1;

      // padding is also done
      *(dst+byte_idx) = (*(dst+byte_idx) & (~0b1 << (BYTE_SIZE-1-bit_idx))) | (input_bit << (BYTE_SIZE-1-bit_idx));
      
      // update indices for next iteration
      bit_idx += 1;
      byte_idx += (int) (bit_idx / BYTE_SIZE);
      bit_idx %= BYTE_SIZE;
    }
  }

  // update indices for next function call
  *dst_byte_idx = byte_idx;
  *dst_bit_idx = bit_idx;

  return 1;
}
