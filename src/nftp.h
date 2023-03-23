// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//
// This is a Customized File Transfer Protocol nftp.
//

#ifndef NANO_FTP_H
#define NANO_FTP_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "log4nftp.h"

#define NFTP_TYPE_HELLO   0x01
#define NFTP_TYPE_ACK     0x02
#define NFTP_TYPE_FILE    0x03
#define NFTP_TYPE_END     0x04
#define NFTP_TYPE_GIVEME  0x05

#define NFTP_SIZE         32
#define NFTP_BLOCK_SZ     (32 * 1024) // Maximal size of single package
#define NFTP_BLOCK_NUM    (0xFFFF) // Maximal number of blocks
#define NFTP_FILES        32 // Receive up to 32 files at once
#define NFTP_HASH(p, n)   nftp_crc32c(p, n)
#define NFTP_FNAME_LEN    64
#define NFTP_FDIR_LEN     256

enum NFTP_ERR {
	NFTP_ERR_HASH = 0x01,
	NFTP_ERR_FILEPATH,
	NFTP_ERR_FILENAME,
	NFTP_ERR_BLOCKS,
	NFTP_ERR_ID,
	NFTP_ERR_CONTENT,
	NFTP_ERR_FILE,
	NFTP_ERR_MEM,
	NFTP_ERR_OVERFLOW,
	NFTP_ERR_EMPTY,
	NFTP_ERR_PROTO,
	NFTP_ERR_DIRTY,
	NFTP_ERR_VEC,
	NFTP_ERR_IOVS,
	NFTP_ERR_FLAG,
	NFTP_ERR_STREAM,
	NFTP_ERR_HT,
	NFTP_ERR_TYPE,
};

enum NFTP_SCHEMA {
	NFTP_SCHEMA_IOVS = 0x01,
	NFTP_SCHEMA_VEC,
};

#define NFTP_HEAD (-1)
#define NFTP_TAIL (0x7FFFFFFF)

enum NFTP_STATUS {
	NFTP_STATUS_HELLO = 0x01,
	NFTP_STATUS_ACK,
	NFTP_STATUS_TRANSFER,
	NFTP_STATUS_END,
	NFTP_STATUS_FINISH,
};

#ifdef _WIN32
struct iovec {
	void * iov_base;
	size_t iov_len;
};
#else
#include <sys/uio.h>
#endif

typedef struct {
	uint8_t   type;
	uint32_t  len;
	uint8_t   id;
	uint16_t  blocks;
	uint16_t  blockseq;
	char *    fpath;
	char *    fname;
	uint16_t  namelen;
	uint32_t  fileid;
	uint32_t  hashcode;
	uint8_t * content;
	size_t    ctlen;
	uint8_t * exbuf;
} nftp;

typedef struct _iter {
	int    schema;
	int    key;
	void * val;
	void * matrix;
	struct _iter *(*next)(struct _iter *);
	struct _iter *(*prev)(struct _iter *);
	void          (*free)(struct _iter *);
} nftp_iter;

#define nftp_file_partname(buf, fname)                \
	do {                                          \
		strcpy(buf, fname);                   \
		strcpy(buf + strlen(fname), ".part"); \
	} while (0)

#define nftp_file_fullpath(buf, dir, fname)               \
	do {                                              \
		if (dir) {                                \
			strcpy(buf, dir);                 \
			strcpy(buf + strlen(dir), fname); \
		} else {                                  \
			strcpy(buf, fname);               \
		}                                         \
	} while (0)

uint32_t nftp_djb_hashn(const uint8_t *, size_t);
uint32_t nftp_fnv1a_hashn(const uint8_t *, size_t);
uint8_t  nftp_crc(const uint8_t *, size_t);
uint32_t nftp_crc32(const uint8_t *, size_t);
uint32_t nftp_crc32c(const uint8_t *, size_t);

char * nftp_file_bname(char *);
char * nftp_file_path(char *);

int nftp_file_exist(char *);
int nftp_file_newname(char *, char **);
int nftp_file_rename(char *, char *);
int nftp_file_size(char *, size_t *);
int nftp_file_blocks(char *, size_t *);
int nftp_file_readblk(char *, int, char **, size_t *);
int nftp_file_read(char *, char **, size_t *);
int nftp_file_write(char *, char *, size_t);
int nftp_file_append(char *, char *, size_t);
int nftp_file_clear(char *);
int nftp_file_hash(char *, uint32_t *);

nftp_iter * nftp_iter_alloc(int, void *);
void        nftp_iter_free(nftp_iter *);
nftp_iter * nftp_iter_next(nftp_iter *);
nftp_iter * nftp_iter_prev(nftp_iter *);

#define NFTP_NEXT(iter) nftp_iter_next(iter)
#define NFTP_PREV(iter) nftp_iter_prev(iter)

typedef struct _vec nftp_vec;

int nftp_vec_alloc(nftp_vec **);
int nftp_vec_free(nftp_vec *);
int nftp_vec_append(nftp_vec *, void *);
int nftp_vec_insert(nftp_vec *, void *, int);
int nftp_vec_delete(nftp_vec *, void **, int);
int nftp_vec_push(nftp_vec *, void *, int);
int nftp_vec_pop(nftp_vec *, void **, int);
int nftp_vec_get(nftp_vec *, int, void **);
int nftp_vec_getidx(nftp_vec *, void *, int*);
int nftp_vec_cat(nftp_vec *, nftp_vec *);
int nftp_vec_cap(nftp_vec *);
int nftp_vec_len(nftp_vec *);
// Iterator
nftp_iter * nftp_vec_iter(nftp_vec *);

typedef struct _iovs nftp_iovs;

int nftp_iovs_alloc(nftp_iovs **);
int nftp_iovs_append(nftp_iovs *, void *, size_t);
int nftp_iovs_insert(nftp_iovs *, void *, size_t, size_t);
int nftp_iovs_push(nftp_iovs *, void *, size_t, int);
int nftp_iovs_pop(nftp_iovs *, void **, size_t *, int);
int nftp_iovs_cat(nftp_iovs *, nftp_iovs *);
int nftp_iovs_free(nftp_iovs *);
size_t nftp_iovs_len(nftp_iovs *);
size_t nftp_iovs_cap(nftp_iovs *);
// Iterator
nftp_iter * nftp_iovs_iter(nftp_iovs *);

int nftp_iovs2stream(nftp_iovs *, uint8_t **, size_t *);

#define nftp_put_u32(ptr, u)                                  \
	do {                                                  \
		(ptr)[0] = (uint8_t)(((uint32_t)(u)) >> 24u); \
		(ptr)[1] = (uint8_t)(((uint32_t)(u)) >> 16u); \
		(ptr)[2] = (uint8_t)(((uint32_t)(u)) >> 8u);  \
		(ptr)[3] = (uint8_t)((uint32_t)(u));          \
	} while (0)

#define nftp_get_u32(ptr, v)                           \
	v = (((uint32_t)((uint8_t)(ptr)[0])) << 24u) + \
	    (((uint32_t)((uint8_t)(ptr)[1])) << 16u) + \
	    (((uint32_t)((uint8_t)(ptr)[2])) << 8u) +  \
	    (((uint32_t)(uint8_t)(ptr)[3]))

#define nftp_put_u16(ptr, u)                                    \
	do {                                                 \
		(ptr)[0] = (uint8_t)(((uint16_t)(u)) >> 8u); \
		(ptr)[1] = (uint8_t)((uint16_t)(u));         \
	} while (0)

#define nftp_get_u16(ptr, v)                             \
	v = (((uint16_t)((uint8_t)(ptr)[0])) << 8u) + \
	    (((uint16_t)(uint8_t)(ptr)[1]))

int nftp_alloc(nftp **);
int nftp_decode_iovs(nftp *, nftp_iovs *);
int nftp_decode(nftp *, uint8_t *, size_t);
int nftp_encode_iovs(nftp *, nftp_iovs *);
int nftp_encode(nftp *, uint8_t **, size_t *);
int nftp_free(nftp *);

int nftp_proto_init();
int nftp_proto_fini();
int nftp_proto_send_start(char *);
int nftp_proto_send_stop(char *);

/*
 * This function is to create a NFTP msg quickly.
 *
 * @fpath, Path to file.
 * @type, NFTP_TYPE.
 * @key, A key to present the session of transmission.
 * @n, The index of block (blockseq).
 * @rmsg, The msg we created.
 * @rlen, The length of rmsg.
 *
 * @return, 0 if no errors. Or please refer to NFTP_ERR.
 */
int nftp_proto_maker(char *fpath, int type, int key,
        int n, char **rmsg, int *rlen);

/*
 * This function is to handle the NFTP msg and return msg caller needed.
 *
 * @msg, Msg we received.
 * @len, Length of msg.
 * @rmsg, Msg we returned if needed.
 * @rlen, Length of rmsg.
 *
 * @return, 0 if no errors. Or please refer to NFTP_ERR.
 */
int nftp_proto_handler(char *msg, int len, char **rmsg, int *rlen);

int nftp_proto_register(char *, int (*cb)(void *), void *);

int nftp_set_recvdir(char *);
int test();

#endif

