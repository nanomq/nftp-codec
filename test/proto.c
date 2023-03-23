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
#include "test.h"

static int test_proto_maker();
static int test_proto_maker_hello();
static int test_proto_maker_ack();
static int test_proto_maker_file();
static int test_proto_maker_end();
static int test_proto_maker_giveme();
static int test_proto_handler();

int
test_proto()
{
	nftp_log("test_proto");

	assert(0 == nftp_proto_init());
	test_proto_maker();
	assert(0 == nftp_proto_fini());

	assert(0 == nftp_proto_init());
	test_proto_handler();
	assert(0 == nftp_proto_fini());

	return (0);
}

static inline int
test_send(char *s, char *v, int len) {
	nftp_log("%s", s); v = v; len = len; return (0);}
static inline int
test_recv(char *s, char **vp, int *lenp) {
	nftp_log("%s", s); vp = vp; lenp = lenp; return (0);}

static inline int
cb_proto_demo(void * arg) {nftp_log("Demo: %s", (char *)arg); return (0);}
static inline int
cb_proto_all(void * arg) {nftp_log("All: %s", (char *)arg); return (0);}

static int
test_proto_handler()
{
	char * fname = "./demo.txt";
	char * r = NULL, * s = NULL;
	int    rlen, slen;
	size_t blocks;
	char * bname = nftp_file_bname(fname);
	int    key;

	assert(bname != NULL);
	key = NFTP_HASH(bname, strlen(bname));

	// For recver
	assert(0 == nftp_proto_register("demo.txt", cb_proto_demo, (void *)"I'm demo recv."));
	assert(0 == nftp_set_recvdir("./build/"));

	// For sender
	assert(0 == nftp_proto_send_start(fname));
	nftp_proto_maker(fname, NFTP_TYPE_HELLO, key, 0, &s, &slen);
	assert(NULL != s);
	assert(0 != slen);
	test_send("HELLO", s, slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For recver
	test_recv("HELLO", &r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL != s); // s is ACK msg
	assert(0 != slen);
	test_send("ACK", s, slen);

	free(r);
	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For sender
	test_recv("ACK", &r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL == s);
	assert(0 == slen);
	free(r);

	// File blocks transferring
	if (s == NULL && slen == 0) {
		assert(0 == nftp_file_blocks(fname, (size_t *)&blocks));
		for (int i=0; i < blocks; ++i) {
			assert(0 == nftp_proto_maker(fname, NFTP_TYPE_FILE, key, i, &s, &slen));
			test_send("END", s, slen);
			// Transfer
			r = s; rlen = slen;
			s = NULL; slen = 0;
		}
	}

	// The packet 0 was lost
	nftp_log("Pakcet 0 lost");
	s = NULL; slen = 0;

	// For recver. Ask a giveme packet.
	nftp_proto_maker(fname, NFTP_TYPE_GIVEME, key, 0, &s, &slen);
	assert(NULL != s);
	assert(0 != slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	// For sender. Handle giveme.
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	free(r);
	test_send("0 Packet", s, slen);

	// Transfer
	r = s; rlen = slen;
	s = NULL; slen = 0;

	assert(0 == nftp_proto_send_stop(fname));

	// For recver
	test_recv("END", &r, &rlen);
	assert(0 == nftp_proto_handler(r, rlen, &s, &slen));
	assert(NULL == s); // s is first (also last) file msg
	assert(0 == slen);
	assert(1 == nftp_file_exist("./build/demo.txt"));

	free(r);
	return (0);
}

static int
test_proto_maker()
{
	test_proto_maker_hello();
	test_proto_maker_ack();
	test_proto_maker_file();
	test_proto_maker_end();
	test_proto_maker_giveme();
	return (0);
}

static int
test_proto_maker_hello()
{
	char *v;
	int   len;
	nftp *p;
	char *str = "It's a demo.\nIt's a demo.\n";
	char *fpath = "./demo.txt";
	char *fname = "demo.txt";
	uint8_t key = 16;

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fpath, NFTP_TYPE_HELLO, key, 1, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_HELLO == p->type);
	assert(len == p->len);
	assert(key == p->id);
	assert(1 == p->blocks);
	assert(strlen(fname) == p->namelen);
	assert(0 == strcmp(fname, p->fname));
	assert(NFTP_HASH((const uint8_t *)str, strlen(str)) == p->hashcode);

	assert(0 == nftp_free(p));
	free(v);
	return (0);
}

static int
test_proto_maker_ack()
{
	nftp * p;
	char * v;
	int    len;
	char * fpath = "./demo.txt";
	char * fname = "demo.txt";
	int    key = 16;

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fpath, NFTP_TYPE_ACK, key, 0, &v, &len));
	assert(0 == nftp_decode(p, (uint8_t *)v, (size_t)len));

	assert(NFTP_TYPE_ACK == p->type);
	assert(len == p->len);
	assert(key == p->id);
	assert(NFTP_HASH((const uint8_t *)fname, strlen(fname)) == p->fileid);

	assert(0 == nftp_free(p));
	free(v);

	return (0);
}

static int
test_proto_maker_file()
{
	nftp * p;
	char * v;
	int    len;
	char * fpath = "./demo.txt";
	char * fname = "demo.txt";
	char * str = "It's a demo.\nIt's a demo.\n";
	uint8_t key = 16;

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fpath, NFTP_TYPE_END, key, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_END == p->type);
	assert(len == p->len);
	assert(0 == p->blockseq);
	assert(NFTP_HASH((const uint8_t *)fname, strlen(fname)) == p->fileid);
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

static int
test_proto_maker_giveme()
{
	nftp * p;
	char * v;
	int    len;
	char * fpath = "./demo.txt";
	char * fname = "demo.txt";
	char * str = "It's a demo.\nIt's a demo.\n";
	uint8_t key = 16;

	assert(0 == nftp_alloc(&p));

	assert(0 == nftp_proto_maker(fpath, NFTP_TYPE_GIVEME, key, 0, &v, &len));
	assert(0 == nftp_decode(p, v, len));

	assert(NFTP_TYPE_GIVEME == p->type);
	assert(len == p->len);
	assert(0 == p->blockseq);
	assert(key == p->fileid);

	assert(0 == nftp_free(p));
	free(v);

	return (0);
}

