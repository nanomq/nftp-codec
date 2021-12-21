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

#include <stdlib.h>
#include <string.h>

#include "nftp.h"
#include "hashtable.h"

HashTable files;
char **   files_reg;
size_t    files_cnt;

struct nctx {
	size_t    len;
	size_t    cap;
	uint8_t **entries;
	char *    filename;
	uint32_t  fileflag;
	int     (*cb)(void *);
	void     *arg;
};

static struct nctx *
nctx_alloc(size_t sz)
{
	struct nctx *n;

	if ((n = malloc(sizeof(struct nctx))) == NULL) {
		return NULL;
	}
	if ((n->entries = malloc(sizeof(void *) * sz)) == NULL) {
		free(n);
		return NULL;
	}

	n->len      = 0;
	n->cap      = sz;
	n->cb       = NULL;
	n->arg      = NULL;
	n->filename = NULL;

	return n;
}

static void
nctx_free(struct nctx * n) {
	if (!n) return;
	if (n->entries)
		free(n->entries);
	free(n);
}

static void
nctx_free_cb(void *k, void *v, void *u)
{
	k = k;
	u = u;
	if (v) {
		nctx_free(v);
	}
}

int
nftp_proto_init()
{
	files_cnt = 0;
	if ((files_reg = malloc(sizeof(char *) * (NFTP_RECV_FILES + 1))) ==
	    NULL) {
		return (NFTP_ERR_MEM);
	}
	ht_setup(&files, sizeof(uint32_t), sizeof(struct ctx*), NFTP_RECV_FILES);

	return (0);
}

int
nftp_proto_fini()
{
	if (files_reg[0])
		free(files_reg[0]);
	for (int i=0; i<files_cnt; i++)
		free(files_reg[i+1]);
	free(files_reg);

	ht_iterate(&files, NULL, nctx_free_cb);
	ht_clear(&files);
	ht_destroy(&files);

	return (0);
}

// Passing the msg encoded in nftp protocol, Don't worry if
// the msg is not comply with the nftp protocol, nftp will
// ignore it.
int
nftp_proto_handler(uint8_t * msg, size_t len, uint8_t ** returnmsg)
{
	nftp * n;
	struct nctx * ctx;
	nftp_alloc(&n);

	nftp_decode(n, msg, len);

	switch (n->type) {
	case NFTP_TYPE_HELLO:
		ctx = nctx_alloc(sizeof(uint8_t *) * n->blocks);
		ctx->filename = n->filename;
		ctx->fileflag = NFTP_HASH((const uint8_t *)n->filename, strlen(n->filename));
		break;

	case NFTP_TYPE_ACK:
	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
	case NFTP_TYPE_GIVEME:
	default:
		break;
	}

	return (0);
}

// nftp_proto_register function is used to determine the files
// to to received. Only the msg with fileflag registered would
// be handled. When all the msgs marked with a fileflag are
// received, the cb(arg) function would be executed.
int
nftp_proto_register(char * filename, int (*cb)(void *), void *arg)
{
	char * str;
	if ((str = malloc(sizeof(char) * (strlen(filename) + 1))) == NULL) {
		return (NFTP_ERR_MEM);
	}
	strcpy(str, filename);
	files_reg[files_cnt+1] = str;
	files_cnt ++;

	if (0 == strcmp("*", filename)) {
		files_cnt --;
	}

	return (0);
}

