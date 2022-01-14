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

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "nftp.h"

static int test_codec_hello();
static int test_codec_ack();
static int test_codec_file();
static int test_codec_end();
static int test_codec_giveme();

int
test_codec()
{
	nftp_log("test_codec");
	test_codec_hello();
	test_codec_ack();
	test_codec_file();
	test_codec_end();

	return (0);
}

static int
test_codec_hello()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_hello[] = {
		0x01, 0x00, 0x00, 0x00, 0x11, 0x00,       // type & length & id
		0x03, 0x00, 0x04, 0x61, 0x62, 0x2e, 0x63, // blocks & file "ab.c"
		0x7c, 0x6d, 0x8b, 0xab,                   // hashval
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_hello, 17));

	assert(NFTP_TYPE_HELLO == p->type);
	assert(17 == p->len);
	assert(0 == p->id);
	assert(3 == p->blocks);
	assert(0 == strcmp("ab.c", p->fname));
	assert(4 == p->namelen);

	assert(0 == nftp_encode(p, &v, &len));
	assert(17 == len);
	for (int i=0; i<len; i++) {
		assert(demo1_hello[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

static int
test_codec_ack()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_ack[] = {
		0x02, 0x00, 0x00, 0x00, 0x0a, 0x00,       // type & length & id
		0x7c, 0x6d, 0x8b, 0xab,                   // fileflag
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_ack, 10));

	assert(NFTP_TYPE_ACK == p->type);
	assert(10 == p->len);
	assert(0 == p->id);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileflag);

	assert(0 == nftp_encode(p, &v, &len));
	assert(10 == len);
	for (int i=0; i<len; i++) {
		assert(demo1_ack[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

static int
test_codec_file()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_file[] = {
		0x03, 0x00, 0x00, 0x00, 0x11, 0x02,       // type & length & id
		0x7c, 0x6d, 0x8b, 0xab,                   // fileflag
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x4f  // content & CRC
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_file, 17));

	assert(NFTP_TYPE_FILE == p->type);
	assert(17 == p->len);
	assert(2 == p->id);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileflag);
	assert(0 == strncmp((char *)(demo1_file + 10), (char *)p->content, 6));

	assert(0 == nftp_encode(p, &v, &len));
	assert(17 == len);
	for (int i=0; i<len; i++) {
		assert(demo1_file[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

static int
test_codec_end()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_file[] = {
		0x04, 0x00, 0x00, 0x00, 0x11, 0x02,       // type & length & id
		0x7c, 0x6d, 0x8b, 0xab,                   // fileflag
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x4f  // content & CRC
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_file, 17));

	assert(NFTP_TYPE_END == p->type);
	assert(17 == p->len);
	assert(2 == p->id);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileflag);
	assert(0 == strncmp((char *)(demo1_file + 10), (char *)p->content, 6));

	assert(0 == nftp_encode(p, &v, &len));
	assert(17 == len);
	for (int i=0; i<len; i++) {
		assert(demo1_file[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);

	return (0);
}

static int
test_codec_giveme()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_giveme[] = {
		0x05, 0x00, 0x00, 0x00, 0x0c, 0x02, // type & length & id
		0x00, 0x04, 0x61, 0x62, 0x2e, 0x63, // file "ab.c"
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_giveme, 12));

	assert(NFTP_TYPE_GIVEME == p->type);
	assert(12 == p->len);
	assert(2 == p->id);
	assert(0 == strcmp("ab.c", p->fname));
	assert(4 == p->namelen);

	assert(0 == nftp_encode(p, &v, &len));
	assert(12 == len);
	for (int i=0; i<len; i++) {
		assert(demo1_giveme[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

