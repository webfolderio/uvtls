/* Copyright Michael A. Penick
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UVTLS_H
#define UVTLS_H

#include <uv.h>
#include <uvtls/internal.h>

/*
 * TODO:
 * - Better error and OOM handling
 * - Provide a way to change supported ciphers
 * - Handle TLS renegotiation
 * - Server certificates chain support
 * - Replace allocator
 * - Support other TLS libraries (boringssl, SChannel, NSS)
 * - Move more core implementation into the shared code
 */

typedef struct uvtls_context_s uvtls_context_t;
typedef struct uvtls_s uvtls_t;
typedef struct uvtls_write_s uvtls_write_t;

typedef void (*uvtls_alloc_cb)(uvtls_t* tls,
                               size_t suggested_size,
                               uv_buf_t* buf);
typedef void (*uvtls_read_cb)(uvtls_t* tls,
                              ssize_t nread,
                              const uv_buf_t* buf);

typedef void (*uvtls_handshake_done_cb)(uvtls_t* tls, int status);
typedef uvtls_handshake_done_cb uvtls_connect_cb;
typedef uvtls_handshake_done_cb uvtls_accept_cb;
typedef void (*uvtls_connection_cb)(uvtls_t* server, int status);

typedef void (*uvtls_close_cb)(uvtls_t* tls);
typedef void (*uvtls_write_cb)(uvtls_write_t* req, int status);


#define UVTLS__ERR(x) (UV_ERRNO_MAX - (x))

#define UVTLS__UNKNOWN UVTLS__ERR(7)
#define UVTLS__EINVAL UVTLS__ERR(6)
#define UVTLS__EHANDSHAKE UVTLS__ERR(5)
#define UVTLS__ENOPEERCERT UVTLS__ERR(4)
#define UVTLS__EBADPEERCERT UVTLS__ERR(3)
#define UVTLS__EBADPEERIDENT UVTLS__ERR(2)
#define UVTLS__EREAD UVTLS__ERR(1)

#define UVTLS_ERRNO_MAP(XX)                    \
  XX(UNKNOWN, "unknown tls error")             \
  XX(EINVAL, "invalid argument")               \
  XX(EHANDSHAKE, "handshake error")            \
  XX(ENOPEERCERT, "no peer certificate")       \
  XX(EBADPEERCERT, "invalid peer certificate") \
  XX(EBADPEERIDENT, "invalid peer identity")   \
  XX(EREAD, "Read error")

typedef enum {
#define XX(code, _) UVTLS_##code = UVTLS__##code,
  UVTLS_ERRNO_MAP(XX)
#undef XX
      UVTLS_ERRNO_MAX = UVTLS__UNKNOWN - 1
} uvtls_errno_t;

struct uvtls_context_s {
  void* data;
  void* impl;
  int verify_flags;
};

struct uvtls_s {
  uv_stream_t* stream;
  void* data;
  void* impl;
  uvtls_context_t* context;
  char hostname[256];
  uvtls_ring_buf_t incoming;
  uvtls_ring_buf_t outgoing;
  uvtls_alloc_cb alloc_cb;
  uv_buf_t alloc_buf;
  uvtls_read_cb read_cb;
  uvtls_handshake_done_cb handshake_done_cb;
  uvtls_connection_cb connection_cb;
  uvtls_ring_buf_pos_t commit_pos;
  uvtls_close_cb close_cb;
};

struct uvtls_write_s {
  uv_write_t req;
  void* data;
  uvtls_t* tls;
  uvtls_write_cb cb;
  uvtls_ring_buf_pos_t commit_pos;
};

typedef enum {
  UVTLS_CONTEXT_LIB_INIT = 0x01,
  UVTLS_CONTEXT_DEBUG = 0x02
} uvtls_context_flags_t;

typedef enum {
  UVTLS_VERIFY_NONE = 0x00,
  UVTLS_VERIFY_PEER_CERT = 0x01,
  UVTLS_VERIFY_PEER_IDENT = 0x02
} uvtls_verify_flags_t;

int uvtls_context_init(uvtls_context_t* context, int init_flags);
void uvtls_context_destroy(uvtls_context_t* context);

void uvtls_context_set_verify_flags(uvtls_context_t* context,
                                    int verify_flags);

int uvtls_context_add_trusted_certs(uvtls_context_t* context,
                                    const char* cert,
                                    size_t length);
int uvtls_context_set_cert(uvtls_context_t* context,
                           const char* cert,
                           size_t length);
int uvtls_context_set_private_key(uvtls_context_t* context,
                                  const char* key,
                                  size_t length);

int uvtls_init(uvtls_t* tls, uvtls_context_t* context, uv_stream_t* stream);

int uvtls_set_hostname(uvtls_t* tls, const char* hostname, size_t length);

int uvtls_connect(uvtls_t* tls, uvtls_connect_cb cb);

int uvtls_is_closing(uvtls_t* tls);
void uvtls_close(uvtls_t* tls, uvtls_close_cb cb);

int uvtls_listen(uvtls_t* tls, int backlog, uvtls_connection_cb cb);

int uvtls_accept(uvtls_t* tls, uvtls_accept_cb cb);

int uvtls_read_start(uvtls_t* tls,
                     uvtls_alloc_cb alloc_cb,
                     uvtls_read_cb read_cb);
int uvtls_read_stop(uvtls_t* tls);

int uvtls_write(uvtls_write_t* req,
                uvtls_t* tls,
                const uv_buf_t bufs[],
                unsigned int nbufs,
                uvtls_write_cb cb);

const char* uvtls_err_name(int err);
#if UV_VERSION_HEX >= 0x011600 /* 1.22.0 */
char* uvtls_err_name_r(int error, char* buf, size_t buflen);
#endif

const char* uvtls_strerror(int err);
#if UV_VERSION_HEX >= 0x011600 /* 1.22.0 */
char* uvtls_strerror_r(int error, char* buf, size_t buflen);
#endif

#endif /* UVTLS_H */
