#include "ring-buf.h"

/*-----------------------------------------------------------------------------
 * MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain. The author hereby disclaims copyright to this source code.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int MD5_u32plus;

typedef struct {
  MD5_u32plus lo, hi;
  MD5_u32plus a, b, c, d;
  unsigned char buffer[64];
  MD5_u32plus block[16];
} MD5_CTX;

/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) (((x) ^ (y)) ^ (z))
#define H2(x, y, z) ((x) ^ ((y) ^ (z)))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s)                        \
  (a) += f((b), (c), (d)) + (x) + (t);                      \
  (a) = (((a) << (s)) | (((a) &0xffffffff) >> (32 - (s)))); \
  (a) += (b);

/*
 * SET reads 4 input bytes in little-endian byte order and stores them in a
 * properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned memory
 * accesses is just an optimization.  Nothing will break if it fails to detect
 * a suitable architecture.
 *
 * Unfortunately, this optimization may be a C strict aliasing rules violation
 * if the caller's data buffer has effective type that cannot be aliased by
 * MD5_u32plus.  In practice, this problem may occur if these MD5 routines are
 * inlined into a calling function, or with future and dangerously advanced
 * link-time optimizations.  For the time being, keeping these MD5 routines in
 * their own translation unit avoids the problem.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define SET(n) (*(MD5_u32plus*) &ptr[(n) *4])
#define GET(n) SET(n)
#else
#define SET(n)                                               \
  (ctx->block[(n)] = (MD5_u32plus) ptr[(n) *4] |             \
                     ((MD5_u32plus) ptr[(n) *4 + 1] << 8) |  \
                     ((MD5_u32plus) ptr[(n) *4 + 2] << 16) | \
                     ((MD5_u32plus) ptr[(n) *4 + 3] << 24))
#define GET(n) (ctx->block[(n)])
#endif

/*
 * This processes one or more 64-byte data blocks, but does NOT update the bit
 * counters.  There are no alignment requirements.
 */
static const void* body(MD5_CTX* ctx, const void* data, unsigned long size) {
  const unsigned char* ptr;
  MD5_u32plus a, b, c, d;
  MD5_u32plus saved_a, saved_b, saved_c, saved_d;

  ptr = (const unsigned char*) data;

  a = ctx->a;
  b = ctx->b;
  c = ctx->c;
  d = ctx->d;

  do {
    saved_a = a;
    saved_b = b;
    saved_c = c;
    saved_d = d;

    /* Round 1 */
    STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
    STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
    STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
    STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
    STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
    STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
    STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
    STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
    STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
    STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
    STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
    STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
    STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
    STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
    STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
    STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

    /* Round 2 */
    STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
    STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
    STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
    STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
    STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
    STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
    STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
    STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
    STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
    STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
    STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
    STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
    STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
    STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
    STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
    STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

    /* Round 3 */
    STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
    STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11)
    STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
    STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23)
    STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
    STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11)
    STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
    STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23)
    STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
    STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11)
    STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
    STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23)
    STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
    STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11)
    STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
    STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23)

    /* Round 4 */
    STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
    STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
    STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
    STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
    STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
    STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
    STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
    STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
    STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
    STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
    STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
    STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
    STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
    STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
    STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
    STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

    a += saved_a;
    b += saved_b;
    c += saved_c;
    d += saved_d;

    ptr += 64;
  } while (size -= 64);

  ctx->a = a;
  ctx->b = b;
  ctx->c = c;
  ctx->d = d;

  return ptr;
}

void MD5_Init(MD5_CTX* ctx) {
  ctx->a = 0x67452301;
  ctx->b = 0xefcdab89;
  ctx->c = 0x98badcfe;
  ctx->d = 0x10325476;

  ctx->lo = 0;
  ctx->hi = 0;
}

void MD5_Update(MD5_CTX* ctx, const void* data, unsigned long size) {
  MD5_u32plus saved_lo;
  unsigned long used, available;

  saved_lo = ctx->lo;
  if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
    ctx->hi++;
  ctx->hi += size >> 29;

  used = saved_lo & 0x3f;

  if (used) {
    available = 64 - used;

    if (size < available) {
      memcpy(&ctx->buffer[used], data, size);
      return;
    }

    memcpy(&ctx->buffer[used], data, available);
    data = (const unsigned char*) data + available;
    size -= available;
    body(ctx, ctx->buffer, 64);
  }

  if (size >= 64) {
    data = body(ctx, data, size & ~(unsigned long) 0x3f);
    size &= 0x3f;
  }

  memcpy(ctx->buffer, data, size);
}

#define OUT(dst, src)                       \
  (dst)[0] = (unsigned char) (src);         \
  (dst)[1] = (unsigned char) ((src) >> 8);  \
  (dst)[2] = (unsigned char) ((src) >> 16); \
  (dst)[3] = (unsigned char) ((src) >> 24);

void MD5_Final(unsigned char* result, MD5_CTX* ctx) {
  unsigned long used, available;

  used = ctx->lo & 0x3f;

  ctx->buffer[used++] = 0x80;

  available = 64 - used;

  if (available < 8) {
    memset(&ctx->buffer[used], 0, available);
    body(ctx, ctx->buffer, 64);
    used = 0;
    available = 64;
  }

  memset(&ctx->buffer[used], 0, available - 8);

  ctx->lo <<= 3;
  OUT(&ctx->buffer[56], ctx->lo)
  OUT(&ctx->buffer[60], ctx->hi)

  body(ctx, ctx->buffer, 64);

  OUT(&result[0], ctx->a)
  OUT(&result[4], ctx->b)
  OUT(&result[8], ctx->c)
  OUT(&result[12], ctx->d)

  memset(ctx, 0, sizeof(*ctx));
}

int rand_range(int min, int max) {
  int delta = max - min;
  assert(max >= min && "Max should be greater than min");
  if (delta == 0) {
    return min;
  }
  return min +
         (int) rand() % delta; /* Not perfect distribution but good enough */
}

void fill_random(char* buf, int length) {
  int i;
  for (i = 0; i < length; ++i) {
    buf[i] = (char) 97 + rand() % 26;
  }
  buf[length] = '\0';
}

#define MAX_LENGTH (1024 * 1024)
/*#define MAX_LENGTH (64 * 1024)*/

void test_write(uvtls_ring_buf_t* rb, const char* buf, int length) {
  const char* pos = buf;
  int remaining = length;

  while (remaining > 0) {
    int to_copy = rand_range(1, remaining);
    if (to_copy > remaining) {
      to_copy = remaining;
    }
    uvtls_ring_buf_write(rb, pos, to_copy);
    remaining -= to_copy;
    pos += to_copy;
  }
}

void test_read(uvtls_ring_buf_t* rb, int length, unsigned char md5[]) {
  MD5_CTX ctx;
  int num_bytes;
  int remaining = length;
  char* temp = (char*) malloc(length + 1);

  MD5_Init(&ctx);

  /*
  while (remaining > 0) {
    int to_copy = rand_range(1, remaining);
    if (to_copy > remaining) {
      to_copy = remaining;
    }
    int num_bytes = uvtls_ring_buf_read(rb, temp, to_copy);
    temp[num_bytes] = '\0';
    // printf("%s", temp);
    MD5_Update(&ctx, temp, num_bytes);
    remaining -= to_copy;
  }
  */

  while ((num_bytes = uvtls_ring_buf_read(rb, temp, rand_range(1, length))) >
         0) {
    temp[num_bytes] = '\0';
    MD5_Update(&ctx, temp, num_bytes);
  }

  MD5_Final(md5, &ctx);

  free(temp);
}

void test_write_read(uvtls_ring_buf_t* rb, const char* buf) {
  int i;
  for (i = 1; i <= 100000; ++i) {
    int length = rand_range(1, MAX_LENGTH);
    unsigned char actual[16];
    unsigned char expected[16];

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, buf, length);
    MD5_Final(expected, &ctx);

    test_write(rb, buf, length);
    test_read(rb, length, actual);

    if (memcmp(expected, actual, sizeof(expected)) != 0) {
      printf("failed %d (length %d)\n", i, length);
      abort();
    } else {
      printf("success %d (length %d)\n", i, length);
    }
  }
}

void test_tail_commit(uvtls_ring_buf_t* rb, const char* buf, int length) {
  const char* pos = buf;
  int remaining = length;

  while (remaining > 0) {
    char* temp;
    int available = uvtls_ring_buf_tail_block(rb, &temp, MAX_LENGTH);

    int to_copy = rand_range(1, remaining);
    if (to_copy > remaining) {
      to_copy = remaining;
    }
    if (to_copy > available) {
      to_copy = available;
    }

    memcpy(temp, pos, (unsigned) to_copy);

    uvtls_ring_buf_tail_block_commit(rb, to_copy);

    remaining -= to_copy;
    pos += to_copy;
  }
}

void test_head_commit(uvtls_ring_buf_t* rb, int length, unsigned char md5[]) {
  MD5_CTX ctx;
  int remaining = length;
  char* temp = (char*) malloc(length + 1);

  MD5_Init(&ctx);

  while (remaining > 0) {
    uv_buf_t bufs[3];
    int bufs_count = 3;
    uvtls_ring_buf_pos_t pos =
        uvtls_ring_buf_head_blocks(rb, rb->head, bufs, &bufs_count);

    /*
    int available = 0;
    for (int i = 0; i < buf_count; ++i) {
      available += bufs[i].len;
    }
    int to_copy = rand_range(1, remaining);
    if (to_copy > remaining) {
      to_copy = remaining;
    }
    if (to_copy > available) {
      to_copy = available;
    }

    int i = 0;
    int r = to_copy;
    while (r > 0) {
      int n = bufs[i].len;
      if (n > r) {
        n = r;
      }
      MD5_Update(&ctx, bufs[i].base, n);
      r -= n;
      i++;
    }
    */

    int i;
    int to_copy = 0;
    for (i = 0; i < bufs_count; ++i) {
      to_copy += bufs[i].len;
      MD5_Update(&ctx, bufs[i].base, bufs[i].len);
    }

    uvtls_ring_buf_head_blocks_commit(rb, pos);

    remaining -= to_copy;
  }

  MD5_Final(md5, &ctx);

  free(temp);
}

void test_tail_head_commit(uvtls_ring_buf_t* rb, const char* buf) {
  int i;
  for (i = 1; i <= 10000; ++i) {
    int length = rand_range(1, MAX_LENGTH);

    unsigned char expected[16];
    unsigned char actual[16];

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, buf, length);
    MD5_Final(expected, &ctx);

    test_tail_commit(rb, buf, length);
    test_head_commit(rb, length, actual);

    if (memcmp(expected, actual, sizeof(expected)) != 0) {
      printf("failed %d (length %d)\n", i, length);
      abort();
    } else {
      printf("success %d (length %d)\n", i, length);
    }
  }
}

void test_ring_buf() {
  uvtls_ring_buf_t rb;
  char* buf;
  srand(0x12345679);

  uvtls_ring_buf_init(&rb);

  buf = (char*) malloc(MAX_LENGTH + 1);
  fill_random(buf, MAX_LENGTH);

  /* test_write_read(&rb, buf); */
  test_tail_head_commit(&rb, buf);

  free(buf);
  uvtls_ring_buf_destroy(&rb);
}

int main() {
  test_ring_buf();
  return 0;
}
