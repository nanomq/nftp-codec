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
	char *str = "aaa\n";

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker("demo.txt", NFTP_TYPE_HELLO, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_HELLO == p->type);
	assert(len == p->len);
	assert(0 == p->id);
	assert(1 == p->blocks);
	assert(0 == strcmp("demo.txt", p->filename));
	assert(NFTP_HASH((const uint8_t *)str, strlen(str) + 1) == p->hashcode);

	return (0);
}

static int
test_proto_make_ack()
{

	return (0);
}

static int
test_proto_make_file()
{

	return (0);
}

static int
test_proto_make_end()
{

	return (0);
}

