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
#include "test.h"

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
	test_codec_giveme();

	return (0);
}

static int
test_codec_hello()
{
	nftp * p;
	size_t len;
	uint8_t *v;

	uint8_t demo1_hello[] = {
		0x01, 0x00, 0x00, 0x00, 0x12, 0x00, // type & length & id
		0x00, 0x03, 0x00, 0x04,             // blocks & length of filename
		0x61, 0x62, 0x2e, 0x63,             // filename
		0x7c, 0x6d, 0x8b, 0xab,             // hashval
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_hello, sizeof(demo1_hello)));

	assert(NFTP_TYPE_HELLO == p->type);
	assert(sizeof(demo1_hello) == p->len);
	assert(0 == p->id);
	assert(3 == p->blocks);
	assert(0 == strcmp("ab.c", p->fname));
	assert(4 == p->namelen);

	assert(0 == nftp_encode(p, &v, &len));
	assert(sizeof(demo1_hello) == len);
	for (size_t i=0; i<len; i++) {
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
		0x7c, 0x6d, 0x8b, 0xab,                   // fileid
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_ack, 10));

	assert(NFTP_TYPE_ACK == p->type);
	assert(10 == p->len);
	assert(0 == p->id);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileid);

	assert(0 == nftp_encode(p, &v, &len));
	assert(sizeof(demo1_ack) == len);
	for (size_t i=0; i<len; i++) {
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
		0x03, 0x00, 0x00, 0x00, 0x13,       // type & length
		0x7c, 0x6d, 0x8b, 0xab,             // fileid
		0x00, 0x02, 0x00, 0x06,             // blockseq & length of content
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66  // content
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_file, sizeof(demo1_file)));

	assert(NFTP_TYPE_FILE == p->type);
	assert(sizeof(demo1_file) == p->len);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileid);
	assert(2 == p->blockseq);
	assert(6 == p->ctlen);
	assert(0 == strncmp((char *)(demo1_file + 13), (char *)p->content, 6));

	assert(0 == nftp_encode(p, &v, &len));
	assert(sizeof(demo1_file) == len);
	for (size_t i=0; i<len; i++) {
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

	uint8_t demo1_end[] = {
		0x04, 0x00, 0x00, 0x00, 0x13,       // type & length
		0x7c, 0x6d, 0x8b, 0xab,             // fileid
		0x00, 0x02, 0x00, 0x06,             // blockseq & length of content
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66  // content
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_end, sizeof(demo1_end)));

	assert(NFTP_TYPE_END == p->type);
	assert(sizeof(demo1_end) == p->len);
	assert(((0x7c << 24) + (0x6d << 16) + (0x8b << 8) + (0xab)) == p->fileid);
	assert(0 == strncmp((char *)(demo1_end + 13), (char *)p->content, 6));

	assert(0 == nftp_encode(p, &v, &len));
	assert(sizeof(demo1_end) == len);
	for (size_t i=0; i<len; i++) {
		assert(demo1_end[i] == v[i]);
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
		0x05, 0x00, 0x00, 0x00, 0x0c, 0x02, // type & length
		0x00, 0x04, 0x61, 0x62, 0x2e, 0x63, // file "ab.c"
	};

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_decode(p, demo1_giveme, 12));

	assert(NFTP_TYPE_GIVEME == p->type);
	assert(12 == p->len);
	assert(2 == p->id);
	assert(0 == strcmp("ab.c", p->fname));
	assert(4 == p->namelen);

	/*
	assert(0 == nftp_encode(p, &v, &len));
	assert(12 == len);
	for (size_t i=0; i<len; i++) {
		assert(demo1_giveme[i] == v[i]);
	}

	assert(0 == nftp_free(p));
	free(v);
	*/
	return (0);
}

