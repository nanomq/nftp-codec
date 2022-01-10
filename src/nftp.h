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

#define NFTP_TYPE_HELLO   0x01
#define NFTP_TYPE_ACK     0x02
#define NFTP_TYPE_FILE    0x03
#define NFTP_TYPE_END     0x04

#define NFTP_TYPE_GIVEME  0x05 // TODO

#define NFTP_SIZE         32
#define NFTP_BLOCK_SZ     (256 * 1024) // Maximal size of single package
#define NFTP_FILES        32 // Receive up to 32 files at once
#define NFTP_HASH(p, n)   nftp_djb_hashn(p, n)
#define NFTP_FNAME_LEN    64

#ifdef _WIN32
#ifndef DEBUG
#define nftp_fatal(format, ...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
	} while (0)
#else
// Only EXIT in DEBUG MODE
#define nftp_fatal(format, ...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, __VA_ARGS__);                                     \
		exit(0);                                                      \
	} while (0)
#endif
#else
#ifndef DEBUG
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
	} while (0)
#else
// Only EXIT in DEBUG MODE
#define nftp_fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
		exit(0);                                                      \
	} while (0)
#endif
#endif

#ifdef _WIN32
#define nftp_log(format, ...)                                           \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, __VA_ARGS__)
#else
#define nftp_log(format, arg...)                                           \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, ##arg)
#endif

enum NFTP_ERR {
	NFTP_ERR_HASH = 0x01,
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
	NFTP_ERR_IOVS,
	NFTP_ERR_STREAM,
	NFTP_ERR_HT,
};

enum NFTP_FLAG {
	NFTP_HEAD = 0x01,
	NFTP_TAIL,
};

enum NFTP_CLIENT_TYPE {
	NFTP_SENDER = 0x01,
	NFTP_RECVER,
};

enum NFTP_STATUS {
	NFTP_STATUS_HELLO = 0x01,
	NFTP_STATUS_ACK,
	NFTP_STATUS_TRANSFER,
	NFTP_STATUS_END,
	NFTP_STATUS_FINISH,
};

typedef struct {
	uint8_t   type;
	uint32_t  len;
	uint8_t   id;
	uint8_t   blocks;
	char *    filename;
	uint16_t  namelen;
	uint32_t  fileflag;
	uint32_t  hashcode;
	uint8_t * content;
	size_t    ctlen;
	uint8_t   crc;
	uint8_t * exbuf;
} nftp;

#define nftp_file_partname(fname, buf)                \
	do {                                          \
		strcpy(buf, fname);                   \
		strcpy(buf + strlen(fname), ".part"); \
	} while (0);

#define nftp_set(nftp, key, val)                                 \
	do {                                                           \
		if (offsetof(nftp, crc) == offsetof(nftp, key)) {      \
			fatal("Macro is not supported for CRC Field"); \
		}                                                      \
		nftp->key = val;                                       \
	} while (0)

uint32_t nftp_djb_hashn(const uint8_t *, size_t);
uint32_t nftp_fnv1a_hashn(const uint8_t *, size_t);
uint8_t  nftp_crc(uint8_t *, size_t);

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
int nftp_file_hash(char *fname, uint32_t *);

typedef struct nftp_iovs nftp_iovs;

int nftp_iovs_alloc(nftp_iovs **);
int nftp_iovs_append(nftp_iovs *, void *, size_t);
int nftp_iovs_insert(nftp_iovs *, void *, size_t, size_t);
int nftp_iovs_push(nftp_iovs *, void *, size_t, int);
int nftp_iovs_pop(nftp_iovs *, void **, size_t *, int);
int nftp_iovs_cat(nftp_iovs *, nftp_iovs *);
int nftp_iovs_free(nftp_iovs *);
size_t nftp_iovs_len(nftp_iovs *);
size_t nftp_iovs_cap(nftp_iovs *);

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
int nftp_proto_maker(char *, int, size_t, uint8_t **, size_t *);
int nftp_proto_handler(uint8_t *, size_t, uint8_t **, size_t *);
int nftp_proto_register(char *, int (*cb)(void *), void *, int);

#endif

