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

struct file_cb {
	char *filename;
	int (*cb)(void *);
	void *arg;
};

struct buf {
	uint8_t * body;
	size_t    len;
};

HashTable files;
struct file_cb **fcb_reg;
size_t    fcb_cnt;

struct nctx {
	size_t    len;
	size_t    cap;
	struct buf *entries;
	uint32_t  fileflag;
	uint32_t  hashcode;
	struct file_cb * fcb;
	enum NFTP_STATUS status;
};

static struct nctx *
nctx_alloc(size_t sz)
{
	struct nctx *n;

	if ((n = malloc(sizeof(struct nctx))) == NULL) {
		return NULL;
	}
	if ((n->entries = malloc(sizeof(struct buf) * sz)) == NULL) {
		free(n);
		return NULL;
	}

	n->len      = 0;
	n->cap      = sz;
	n->fcb      = NULL;

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
	fcb_cnt = 0;
	if ((fcb_reg = malloc(sizeof(struct file_cb *) * (NFTP_RECV_FILES + 1))) == NULL) {
		return (NFTP_ERR_MEM);
	}
	memset(fcb_reg, 0, NFTP_RECV_FILES + 1);
	ht_setup(&files, sizeof(uint32_t), sizeof(struct ctx*), NFTP_RECV_FILES);

	return (0);
}

int
nftp_proto_fini()
{
	if (fcb_reg[0]) {
		if (fcb_reg[0]->filename) free(fcb_reg[0]->filename);
		free(fcb_reg[0]);
	}
	for (int i=0; i<fcb_cnt; i++) {
		if (fcb_reg[i+1]) {
			if (fcb_reg[i+1]->filename) free(fcb_reg[i+1]->filename);
			free(fcb_reg[i+1]);
		}
	}
	free(fcb_reg);

	ht_iterate(&files, NULL, nctx_free_cb);
	ht_clear(&files);
	ht_destroy(&files);

	return (0);
}

int
nftp_proto_maker(char *fname, int type, size_t n, uint8_t **rmsg, size_t *rlen)
{
	int rv;
	nftp * p;
	uint8_t *v;
	size_t len;

	if (NULL == fname) {
		return (NFTP_ERR_FILENAME);
	}

	if (0 != (rv = nftp_alloc(&p))) return rv;

	switch (type) {
	case NFTP_TYPE_HELLO:
		p->type = NFTP_TYPE_HELLO;
		p->len = 6 + 1 + 2+strlen(fname) + 4;
		p->id = 0;
		if (0 != (rv = nftp_file_size(fname, &len))) return rv;
		p->blocks = (len/NFTP_BLOCK_SZ) + 1;
		p->filename = fname;
		p->namelen = strlen(fname);

		nftp_file_hash(fname, &p->hashcode);
		break;

	case NFTP_TYPE_ACK:
		p->type = NFTP_TYPE_ACK;
		p->len = 6 + 4;
		p->id = 0;
		p->fileflag = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		nftp_file_read(fname, (char **)&v, &len); //TODO optimization
		if (NFTP_BLOCK_SZ * n < len && len < NFTP_BLOCK_SZ * (n+1)) {
			p->type = NFTP_TYPE_END;
			p->ctlen = len - NFTP_BLOCK_SZ * n;
		}
		if (NFTP_BLOCK_SZ * (n+1) < len) {
			p->type = NFTP_TYPE_FILE;
			p->ctlen = NFTP_BLOCK_SZ;
		}

		p->len = 6 + 4 + p->ctlen + 1;
		p->id = n+1;
		p->fileflag = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		p->content = malloc(p->ctlen);
		memcpy(p->content, v + NFTP_BLOCK_SZ*n, p->ctlen);
		free(v);
		break;

	case NFTP_TYPE_GIVEME:
	default:
		fatal("NOT SUPPORTED");
		break;
	}

	if (0 != (rv = nftp_encode(p, rmsg, rlen))) return rv;

	p->filename = NULL; // Avoid free in nftp_free by mistake
	nftp_free(p);
	return (0);
}

// Passing the msg encoded in nftp protocol, Don't worry if
// the msg is not comply with the nftp protocol, nftp will
// ignore it.
int
nftp_proto_handler(uint8_t * msg, size_t len)
{
	struct nctx * ctx;
	nftp * n;
	nftp_alloc(&n);

	nftp_decode(n, msg, len);

	switch (n->type) {
	case NFTP_TYPE_HELLO:
		ctx = nctx_alloc(sizeof(uint8_t *) * n->blocks);
		ctx->fileflag = NFTP_HASH((const uint8_t *)n->filename,
		        strlen(n->filename));
		ctx->hashcode = n->hashcode;
		for (int i=0; i<fcb_cnt; ++i) {
			if (0 == strcmp(fcb_reg[i+1]->filename, n->filename)) {
				ctx->fcb = fcb_reg[i+1];
			}
		}
		ht_insert(&files, &ctx->fileflag, ctx);
		ctx->status = NFTP_STATUS_HELLO;
		break;

	case NFTP_TYPE_ACK:
		if (!ht_contains(&files, &n->fileflag)) {
			break;
		}
		ctx = (struct nctx *)ht_lookup(&files, &n->fileflag);
		ctx->status = NFTP_STATUS_ACK;
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (!ht_contains(&files, &n->fileflag)) {
			break;
		}
		ctx = (struct nctx *)ht_lookup(&files, &n->fileflag);

		ctx->entries[ctx->len].body = n->content;
		ctx->entries[ctx->len].len = n->ctlen;
		ctx->len ++;

		if (n->type == NFTP_TYPE_FILE) ctx->status = NFTP_STATUS_TRANSFER;
		if (n->type == NFTP_TYPE_END) ctx->status = NFTP_STATUS_END;

		if (ctx->len == ctx->cap) {
			ctx->status = NFTP_STATUS_FINISH;
			ctx->fcb->cb(ctx->fcb->arg);
			// TODO hash check

			// Free resource
			for (int i=0; i<ctx->cap; i++) {
				nftp_file_append(ctx->fcb->filename,
				        (char *)ctx->entries[i].body, ctx->entries[i].len);
			}
			free(ctx->fcb->filename);
			free(ctx->fcb);
			nctx_free(ctx);
		}
		break;

	case NFTP_TYPE_GIVEME:
	default:
		fatal("NOT SUPPORTED");
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
	struct file_cb * fcb;

	if ((fcb = malloc(sizeof(struct file_cb))) == NULL) {
		return (NFTP_ERR_MEM);
	}
	if ((str = malloc(sizeof(char) * (strlen(filename) + 1))) == NULL) {
		return (NFTP_ERR_MEM);
	}

	strcpy(str, filename);
	fcb->filename = str;
	fcb->cb = cb;
	fcb->arg = arg;

	if (0 == strcmp("*", filename)) {
		fcb_reg[0] = fcb;
	} else {
		fcb_reg[fcb_cnt+1] = fcb;
		fcb_cnt ++;
	}

	return (0);
}

