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

static int test_proto_maker();
static int test_proto_maker_hello();
static int test_proto_maker_ack();
static int test_proto_maker_file();
static int test_proto_maker_end();
static int test_proto_handler();

int
test_proto()
{
	log("test_proto");

	assert(0 == nftp_proto_init());
	test_proto_maker();
	assert(0 == nftp_proto_fini());

	assert(0 == nftp_proto_init());
	test_proto_handler();
	assert(0 == nftp_proto_fini());

	return (0);
}

static inline int
test_send(uint8_t *v, size_t len) {log("Send it"); v = v; len = len; return (0);}
static inline int
test_recv(uint8_t **vp, size_t *lenp) {log("Recv it"); vp = vp; lenp = lenp; return (0);}

static inline int
cb_proto_demo(void * arg) {log("Demo: %s", (char *)arg); return (0);}
static inline int
cb_proto_all(void * arg) {log("All: %s", (char *)arg); return (0);}

static int
test_proto_handler()
{
	char * fname = "demo.txt";
	uint8_t * r = NULL, * s = NULL;
	size_t rlen, slen;

	// For recver
	assert(0 == nftp_proto_register("*", cb_proto_all, (void *)"Hello."));
	assert(0 == nftp_proto_register("demo.txt", cb_proto_demo, (void *)"I'm demo."));

	// For sender
	assert(0 == nftp_proto_maker(fname, NFTP_TYPE_HELLO, 0, &s, &slen));
	assert(NULL != s);
	assert(0 != slen);
	test_send(s, slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For recver
	test_recv(&r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL != s); // s is ACK msg
	assert(0 != slen);
	test_send(s, slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For sender
	test_recv(&r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL != s); // s is first file msg
	assert(0 != slen);
	test_send(s, slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For recver
	test_recv(&r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL == s); // s is first file msg
	assert(0 == slen);

	return (0);
}

static int
test_proto_maker()
{
	test_proto_maker_hello();
	test_proto_maker_ack();
	test_proto_maker_file();
	test_proto_maker_end();
	return (0);
}

static int
test_proto_maker_hello()
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
test_proto_maker_ack()
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
test_proto_maker_file()
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
test_proto_maker_end()
{
	return test_proto_maker_file();
}

