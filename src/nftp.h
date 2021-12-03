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

#define fatal(format, arg...)                                                    \
	do {                                                                  \
		fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
		    __FUNCTION__, ##arg);                                   \
		exit(0);                                                      \
	} while (0)
#define log(format, arg...)                                              \
	fprintf(stderr, "%s:%d(%s) " format "\n", __FILE__, __LINE__, \
	    __FUNCTION__, ##arg)

enum NFTP_ERR {
	NFTP_ERR_HASH = 0x01,
	NFTP_ERR_FILENAME,
	NFTP_ERR_BLOCKS,
	NFTP_ERR_ID,
	NFTP_ERR_CONTENT,
	NFTP_ERR_FILE,
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

int nftp_file_read(char *, char **);
int nftp_file_write(char *, char *, size_t);
uint32_t nftp_file_hash(char *fname);

#endif

