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

#ifdef _WIN32
#pragma comment(lib, "hashtable-static.lib")
#endif

static char *recvdir = NULL;

struct file_cb {
	char *fname;
	int (*cb)(void *);
	void *arg;
};

struct buf {
	uint8_t * body;
	size_t    len;
};

HashTable        files;
struct file_cb **fcb_reg;
size_t           fcb_cnt;

struct nctx {
	size_t          len;
	size_t          cap;
	size_t          nextid;
	struct buf *    entries;
	uint32_t        fileflag;
	uint32_t        hashcode;
	struct file_cb *fcb;
	char *          wfname;
	uint8_t         status;
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
	for (int i=0; i<sz; ++i) {
		n->entries[i].len = 0;
		n->entries[i].body = NULL;
	}

	n->len      = 0;
	n->cap      = sz;
	n->nextid   = 1;
	n->wfname   = NULL;
	n->fcb      = NULL;

	return n;
}

static void
nctx_free(struct nctx * n) {
	if (!n) return;
	if (n->entries) {
		for (int i=0; i<n->cap; i++)
			if (n->entries[i].body != NULL)
				free(n->entries[i].body);
		free(n->entries);
	}
	if (n->wfname)
		free(n->wfname);
	free(n);
}

static void
nctx_free_cb(void *k, void *v, void *u)
{
	struct nctx ** vp = v;
	k = k;
	u = u;
	if (vp) {
		nctx_free(*vp);
	}
}

int
nftp_proto_init()
{
	fcb_cnt = 0;
	if ((fcb_reg = malloc(sizeof(struct file_cb *) * (NFTP_FILES + 1))) == NULL) {
		return (NFTP_ERR_MEM);
	}
	memset(fcb_reg, 0, NFTP_FILES + 1);
	nftp_proto_register("*", NULL, NULL);

	ht_setup(&files, sizeof(uint32_t), sizeof(struct nctx*), NFTP_FILES);

	return (0);
}

int
nftp_proto_fini()
{
	for (int i=0; i<fcb_cnt+1; i++) {
		if (fcb_reg[i]) {
			if (fcb_reg[i]->fname) free(fcb_reg[i]->fname);
			free(fcb_reg[i]);
		}
	}
	free(fcb_reg);

	ht_iterate(&files, NULL, nctx_free_cb);
	ht_clear(&files);
	ht_destroy(&files);

	if (recvdir) free(recvdir);

	return (0);
}

int
nftp_proto_send_start(char *fpath)
{
	char * fname;
	if (NULL == fpath) return (NFTP_ERR_FILEPATH);
	if ((fname = nftp_file_bname(fpath)) == NULL)
		return (NFTP_ERR_FILEPATH);

	free(fname);
	return (0);
}

int
nftp_proto_send_stop(char *fpath)
{
	// TODO send something to stop the recver
}

int
nftp_proto_maker(char *fpath, int type, size_t n, uint8_t **rmsg, size_t *rlen)
{
	int rv;
	nftp * p;
	uint8_t *v;
	size_t len, blocks;
	char * fname;

	if (NULL == fpath) return (NFTP_ERR_FILEPATH);
	if ((fname = nftp_file_bname(fpath)) == NULL)
		return (NFTP_ERR_FILEPATH);
	if (0 != (rv = nftp_alloc(&p))) return rv;

	switch (type) {
	case NFTP_TYPE_HELLO:
		p->type = NFTP_TYPE_HELLO;
		p->len = 6 + 1 + 2+strlen(fname) + 4;
		p->id = 0;
		if (0 != (rv = nftp_file_size(fpath, &len))) return rv;
		p->blocks = (len/NFTP_BLOCK_SZ) + 1;
		p->fname = fname;
		p->namelen = strlen(fname);

		if (0 != (rv = nftp_file_hash(fpath, &p->hashcode))) return rv;
		break;

	case NFTP_TYPE_ACK:
		p->type = NFTP_TYPE_ACK;
		p->len = 6 + 4;
		p->id = 0;
		p->fileflag = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (0 >= n) return (NFTP_ERR_ID);
		if (0 != (rv = nftp_file_readblk(fpath, n-1, (char **)&v, &len))) {
			return rv;
		}
		if (0 != (rv = nftp_file_blocks(fpath, &blocks))) {
			return rv;
		}
		if (n == blocks) {
			p->type = NFTP_TYPE_END;
		} else {
			p->type = NFTP_TYPE_FILE;
		}

		p->content = v;
		p->ctlen = len;
		p->len = 6 + 4 + p->ctlen + 1;
		p->id = n;
		p->fileflag = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		break;

	case NFTP_TYPE_GIVEME:
	default:
		nftp_fatal("NOT SUPPORTED");
		break;
	}

	if (0 != (rv = nftp_encode(p, rmsg, rlen))) return rv;

	if (NFTP_TYPE_END == p->type) {
		nftp_proto_send_stop(fpath);
	}

	p->fname = NULL; // Avoid free in nftp_free by mistake
	nftp_free(p);
	free(fname);
	return (0);
}

// Passing the msg encoded in nftp protocol, Don't worry if
// the msg is not comply with the nftp protocol, nftp will
// ignore it.
int
nftp_proto_handler(uint8_t * msg, size_t len, uint8_t **retmsg, size_t *rlen)
{
	int rv = 1;
	struct nctx * ctx = NULL;
	uint32_t hashcode = 0;
	nftp * n;
	char   partname[NFTP_FNAME_LEN + 8];
	char   fullpath[NFTP_FNAME_LEN + NFTP_FDIR_LEN];
	char   fullpath2[NFTP_FNAME_LEN + NFTP_FDIR_LEN];

	if (0 != (rv = nftp_alloc(&n))) return rv;

	// Set default return value
	*retmsg = NULL;
	*rlen = 0;

	if (0 != (rv = nftp_decode(n, msg, len))) return rv;

	switch (n->type) {
	case NFTP_TYPE_HELLO:
		ctx = nctx_alloc(n->blocks);
		ctx->fileflag = NFTP_HASH((const uint8_t *)n->fname,
		        strlen(n->fname));
		ctx->hashcode = n->hashcode;

		for (int i=0; i<fcb_cnt; ++i) {
			if (0 == strcmp(fcb_reg[i+1]->fname, n->fname)) {
				ctx->fcb = fcb_reg[i+1];
			}
		}
		if (NULL == ctx->fcb) {
			nftp_log("Unregistered filename [%s]\n", n->fname);
			nftp_proto_register(n->fname, fcb_reg[0]->cb, fcb_reg[0]->arg);
		}

		// TODO Optimization
		for (int i=0; i<fcb_cnt; ++i) {
			if (0 == strcmp(fcb_reg[i+1]->fname, n->fname)) {
				ctx->fcb = fcb_reg[i+1];
			}
		}

		nftp_file_fullpath(fullpath, recvdir, n->fname);
		if (nftp_file_exist(fullpath)) {
			nftp_file_newname(n->fname, &ctx->wfname);
			nftp_log("File [%s] exists, recver would save to [%s]", n->fname, ctx->wfname);
		} else {
			if ((ctx->wfname = malloc(n->namelen+1)) == NULL) {
				return (NFTP_ERR_MEM);
			}
			strcpy(ctx->wfname, n->fname);
		}
		nftp_file_partname(partname, ctx->wfname);
		nftp_file_fullpath(fullpath, recvdir, partname);
		if (0 != (rv = nftp_file_write(fullpath, "", 0))) { // create file
			nftp_fatal("File write failed [%s]", fullpath);
			return rv;
		}

		if (0 != (rv = ht_insert(&files, &ctx->fileflag, &ctx))) {
			nftp_fatal("Error in hash");
			return (NFTP_ERR_HT);
		}
		ctx->status = NFTP_STATUS_HELLO;

		nftp_proto_maker(n->fname, NFTP_TYPE_ACK, 0, retmsg, rlen);
		break;

	case NFTP_TYPE_ACK:
		// TODO return An Iterator
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (!ht_contains(&files, &n->fileflag)) {
			nftp_fatal("Not found fileflag [%d]", n->fileflag);
			return NFTP_ERR_HT;
		}
		ctx = *((struct nctx **)ht_lookup(&files, &n->fileflag));

		nftp_file_partname(partname, ctx->wfname);
		nftp_file_fullpath(fullpath, recvdir, partname);

		if (n->id == ctx->nextid) {
			if (0 != nftp_file_append(fullpath, n->content, n->ctlen)) {
				nftp_fatal("Error in file append [%s]", fullpath);
				return (NFTP_ERR_FILE);
			}
			do {
				ctx->nextid ++;
				if ((ctx->nextid > ctx->cap-1) ||
				    (ctx->entries[ctx->nextid].body == NULL))
					break;
				if (0 != nftp_file_append(fullpath,
				        ctx->entries[ctx->nextid].body,
				        ctx->entries[ctx->nextid].len)) {
					nftp_fatal("Error in file append [%s]", fullpath);
					return (NFTP_ERR_FILE);
				}
				free(ctx->entries[ctx->nextid].body);
				ctx->entries[ctx->nextid].body = NULL;
				ctx->entries[ctx->nextid].len  = 0;
			} while (1);
		} else {
			// Just store it
			ctx->entries[n->id].len = n->ctlen;
			ctx->entries[n->id].body = n->content;
			n->content = NULL; // avoid be free
		}

		ctx->len ++;
		nftp_log("Process(recv) [%s]:[%ld/%ld]\n",
			ctx->wfname, ctx->len, ctx->cap);

		if (n->type == NFTP_TYPE_FILE) ctx->status = NFTP_STATUS_TRANSFER;
		if (n->type == NFTP_TYPE_END) ctx->status = NFTP_STATUS_END;

		// Recved finished
		if (ctx->len == ctx->cap) {
			ctx->status = NFTP_STATUS_FINISH;
			// Rename
			nftp_file_fullpath(fullpath2, recvdir, ctx->wfname);
			if (0 != nftp_file_rename(fullpath, fullpath2)) {
				nftp_fatal("Error happened in file rename [%s].", fullpath);
				return (NFTP_ERR_FILE);
			}
			// hash check
			if (0 != nftp_file_hash(fullpath2, &hashcode)) {
				nftp_fatal("Error happened in file hash [%s].", fullpath2);
				return (NFTP_ERR_FILE);
			} else {
				nftp_log("Hash check passed [%s].", fullpath2);
			}
			if (ctx->hashcode != hashcode) {
				nftp_fatal("Error happened in recving [%s].", ctx->wfname);
				return (NFTP_ERR_PROTO);
			}

			// Run cb
			if (NULL == ctx->fcb) {
				nftp_log("Unregistered.");
				break;
			}
			if (ctx->fcb->cb)
				ctx->fcb->cb(ctx->fcb->arg);

			// Free resource
			for (int i=0; i<fcb_cnt; ++i)
				if (ctx->fcb == fcb_reg[i+1]) {
					fcb_reg[i+1] = NULL;
					fcb_cnt --;
				}
			free(ctx->fcb->fname);
			free(ctx->fcb);

			if (0 != (rv = ht_erase(&files, &ctx->fileflag))) {
				nftp_fatal("Not find the key [%d] in hashtable.", ctx->fileflag);
				return (NFTP_ERR_HT);
			}
			nctx_free(ctx);
		}
		break;

	case NFTP_TYPE_GIVEME:
	default:
		nftp_fatal("NOT SUPPORTED");
		break;
	}
	nftp_free(n);

	return (0);
}

// nftp_proto_register function is used to determine the files
// to to received. Only the msg with fileflag registered would
// be handled. When all the msgs marked with a fileflag are
// received, the cb(arg) function would be executed.
int
nftp_proto_register(char * fname, int (*cb)(void *), void *arg)
{
	struct file_cb * fcb;

	if (NULL == fname) return (NFTP_ERR_FILENAME);

	if ((fcb = malloc(sizeof(struct file_cb))) == NULL) {
		nftp_fatal("Error in malloc fcb.");
		return (NFTP_ERR_MEM);
	}
	fcb->cb = cb;
	fcb->arg = arg;

	if ((fcb->fname = malloc(strlen(fname)+1)) == NULL) {
		return (NFTP_ERR_MEM);
	}
	strcpy(fcb->fname, fname);

	if (0 == strcmp("*", fname)) {
		fcb_reg[0] = fcb;
	} else {
		fcb_reg[fcb_cnt+1] = fcb;
		fcb_cnt ++;
	}

	return (0);
}

int
nftp_set_recvdir(char * dir)
{
	char * rdir;
	if ((rdir = malloc(strlen(dir)+1)) == NULL) {
		return (NFTP_ERR_MEM);
	}
	if (recvdir) free(recvdir);

	strcpy(rdir, dir);
	recvdir = rdir;

	return (0);
}

