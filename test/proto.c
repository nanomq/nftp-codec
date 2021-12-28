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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "nftp.h"

static int test_proto_make_hello();
static int test_proto_make_ack();
static int test_proto_make_file();
static int test_proto_make_end();

int
test_proto()
{
	log("test_proto");

	assert(0 == nftp_proto_init());
	test_proto_make_hello();
	test_proto_make_ack();
	test_proto_make_file();
	test_proto_make_end();
	assert(0 == nftp_proto_fini());
	return (0);
}

static int
test_proto_make_hello()
{
	uint8_t * v;
	size_t len;
	nftp *p;
	char *str = "It's a demo.\nIt's a demo.\n";
	char *fname = "demo.txt";

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fname, NFTP_TYPE_HELLO, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_HELLO == p->type);
	assert(len == p->len);
	assert(0 == p->id);
	assert(1 == p->blocks);
	assert(strlen(fname) == p->namelen);
	assert(0 == strcmp(fname, p->filename));
	assert(NFTP_HASH((const uint8_t *)str, strlen(str)) == p->hashcode);

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

static int
test_proto_make_ack()
{
	nftp *p;
	uint8_t * v;
	size_t len;
	char * fname = "demo.txt";

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fname, NFTP_TYPE_ACK, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_ACK == p->type);
	assert(len == p->len);
	assert(0 == p->id);
	assert(NFTP_HASH((const uint8_t *)fname, strlen(fname)) == p->fileflag);

	assert(0 == nftp_free(p));
	free(v);

	return (0);
}

static int
test_proto_make_file()
{
	nftp *p;
	uint8_t * v;
	size_t len;
	char * fname = "demo.txt";
	char *str = "It's a demo.\nIt's a demo.\n";

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fname, NFTP_TYPE_FILE, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_END == p->type);
	assert(len == p->len);
	assert(1 == p->id);
	assert(NFTP_HASH((const uint8_t *)fname, strlen(fname)) == p->fileflag);
	assert(0 == strncmp(str, (char *)p->content, strlen(str)));

	assert(0 == nftp_free(p));
	free(v);

	return (0);
}

static int
test_proto_make_end()
{
	return test_proto_make_file();
}

