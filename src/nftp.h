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

#ifndef DEBUG
#define fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
	} while (0)
#else
// Only EXIT in DEBUG MODE
#define fatal(format, arg...)                                                 \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                     \
		exit(0);                                                      \
	} while (0)
#endif

#define log(format, arg...)                                           \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, ##arg)

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
};

enum NFTP_FLAG {
	NFTP_HEAD = 0x01,
	NFTP_TAIL,
};

typedef struct {
	uint8_t   type;
	uint32_t  len;
	int       id;
	int       blocks;
	char *    filename;
	uint8_t   namelen;
	uint32_t  fileflag;
	uint8_t * content;
	size_t    ctlen;
	uint8_t   crc;
} nftp;

uint32_t nftp_djb_hashn(const uint8_t *, size_t);
uint32_t nftp_fnv1a_hashn(const uint8_t *, size_t);
uint8_t  nftp_crc(uint8_t *, size_t);

int nftp_file_exist(char *);
int nftp_file_size(char *, size_t *);
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
int nftp_iovs_pop(nftp_iovs *, void *, size_t, int);
int nftp_iovs_cat(nftp_iovs *, nftp_iovs *);
int nftp_iovs_free(nftp_iovs *);
size_t nftp_iovs_len(nftp_iovs *);
size_t nftp_iovs_cap(nftp_iovs *);

#endif

