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
	char* body;
	int   len;
};

HashTable files;
nftp_vec *fcb_reg = NULL;
HashTable senderfiles;

struct nctx {
	int             len;
	int             cap;
	int             nextid;
	struct buf *    entries;
	uint32_t        fileid;
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
	for (size_t i=0; i<sz; ++i) {
		n->entries[i].len = 0;
		n->entries[i].body = NULL;
	}

	n->len      = 0;
	n->cap      = sz;
	n->nextid   = 0;
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
	(void )k;
	(void )u;
	if (vp) {
		nctx_free(*vp);
	}
}

static void
senderfile_free_cb(void *k, void *v, void *u)
{
	(void) k;
	(void) v;
	(void) u;
}

int
nftp_proto_init()
{
	int rv;

	if (0 != (rv = nftp_vec_alloc(&fcb_reg, NFTP_FILES)))
		return rv;
	// Set default callback and arg for all file
	if (0 != (rv = nftp_proto_register("*", NULL, NULL)))
		return rv;

	ht_setup(&files, sizeof(uint32_t), sizeof(struct nctx*), NFTP_FILES);
	ht_setup(&senderfiles, sizeof(uint32_t), NFTP_FNAME_LEN + NFTP_FDIR_LEN, NFTP_FILES);

	return (0);
}

int
nftp_proto_fini()
{
	int rv;
	struct file_cb *fcb;
	while (0 != nftp_vec_len(fcb_reg)) {
		nftp_vec_pop(fcb_reg, (void **)&fcb, NFTP_HEAD);
		free(fcb->fname);
		free(fcb);
	}
	if (0 != (rv = nftp_vec_free(fcb_reg)))
		return rv;

	ht_iterate(&files, NULL, nctx_free_cb);
	ht_clear(&files);
	ht_destroy(&files);

	ht_iterate(&senderfiles, NULL, senderfile_free_cb);
	ht_clear(&senderfiles);
	ht_destroy(&senderfiles);

	if (recvdir) {
		free(recvdir);
		recvdir = NULL;
	}

	return (0);
}

int
nftp_proto_hello_get_fname(char *rmsg, int rlen, char **fnamep, int *lenp)
{
	(void) rlen;
	if (rmsg[0] != NFTP_TYPE_HELLO)
		return NFTP_ERR_TYPE;
	int pos = 8;
	uint16_t namelen;
	nftp_get_u16(rmsg+pos, namelen);
	char *fname = malloc(namelen + 1);
	strncpy(fname, rmsg + pos + 2, namelen);
	fname[namelen] = '\0';

	*fnamep = fname;
	*lenp   = namelen;
	return 0;
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
	(void) fpath;
	// TODO send something to stop the recver
	return 0;
}

int
nftp_proto_recv_stop(char *fname)
{
	int             rv;
	struct nctx    *ctx = NULL;
	struct file_cb *fcb = NULL;
	char            partname[NFTP_FNAME_LEN + 8];
	char            fullpath[NFTP_FNAME_LEN + NFTP_FDIR_LEN];

	/* Remove '.','/',etc */
	if ((fname = nftp_file_bname(fname)) == NULL)
		return (NFTP_ERR_FILENAME);

	uint32_t fileid = NFTP_HASH((const uint8_t *)fname, strlen(fname));
	// Get ctx
	if (!ht_contains(&files, &fileid)) {
		nftp_fatal("Not found fileid [%d]", fileid);
		free(fname);
		return NFTP_ERR_HT;
	}
	ctx = *((struct nctx **)ht_lookup(&files, &fileid));

	// Get part file
	nftp_file_partname(partname, ctx->wfname);
	nftp_file_fullpath(fullpath, recvdir, partname);

	// Remove ctx from files
	if (0 != ht_erase(&files, &fileid)) {
		nftp_fatal("Not find the key [%d] in hashtable.", fileid);
		free(fname);
		return (NFTP_ERR_HT);
	}

	// Remove part file
	rv = nftp_file_remove(fullpath);
	if (0 != rv) {
		nftp_fatal("Remove file failed [%s]", fullpath);
		free(fname);
		return rv;
	}

	// Remove the fcb
	if (NULL != ctx->fcb) {
		for (int i=0; i<nftp_vec_len(fcb_reg); ++i)
			if (0 == nftp_vec_get(fcb_reg, i, (void **)&fcb))
				if (ctx->fcb == fcb) {
					if (i == 0)
						break;
					rv = nftp_vec_delete(fcb_reg, (void **)&fcb, i);
					if (0 != rv) {
						nftp_fatal("Remove fcb failed [%d]", i);
						return rv;
					}
					if (fcb) {
						free(fcb->fname);
						free(fcb);
					}
					break;
				}
	}

	// Free the ctx and the cached entries
	nctx_free(ctx);
	free(fname);
	return 0;
}

int
nftp_proto_recv_status(char *fname, int *capp, int *nextseq)
{
	struct nctx *ctx;
	uint32_t fileid;

	/* Remove '.','/',etc */
	if ((fname = nftp_file_bname(fname)) == NULL)
		return (NFTP_ERR_FILENAME);

	fileid = NFTP_HASH((uint8_t *)fname, strlen(fname));

	if (!ht_contains(&files, &fileid)) {
		nftp_log("Not found fileid [%d]", fileid);
		*nextseq = -1;
		free(fname);
		return NFTP_ERR_HT;
	}
	ctx = *((struct nctx **)ht_lookup(&files, &fileid));

	*capp = ctx->cap;
	*nextseq = ctx->nextid;

	free(fname);
	return 0;
}

int
nftp_proto_maker(char *fpath, int type, int key, int n, char **rmsg, int *rlen)
{
	int rv;
	nftp * p;
	size_t len, blocks;
	char *v, *fname;
	char  fullpath[NFTP_FNAME_LEN + NFTP_FDIR_LEN];

	if (NULL == fpath) return (NFTP_ERR_FILEPATH);
	if ((fname = nftp_file_bname(fpath)) == NULL)
		return (NFTP_ERR_FILEPATH);
	if (0 != (rv = nftp_alloc(&p))) return rv;

	switch (type) {
	case NFTP_TYPE_HELLO:
		p->type = NFTP_TYPE_HELLO;
		p->len = 5 + 1 + 2 + 2 + strlen(fname) + 4;
		p->id = 0xff & key;
		if (0 != (rv = nftp_file_size(fpath, &len)))
			return rv;

		blocks = (len/NFTP_BLOCK_SZ) + 1;
		if (blocks > (0xFFFF)) {
			nftp_log("File is too large (MAXSIZE: %dKB).",
			    (NFTP_BLOCK_SZ * NFTP_BLOCK_NUM / 1024));
			return NFTP_ERR_BLOCKS;
		}
		p->blocks = (uint16_t)blocks;
		p->fname = fname;
		p->namelen = strlen(fname);

		if (0 != (rv = nftp_file_hash(fpath, &p->hashcode)))
			return rv;

		// XXX bug here. Here we donot get the fileid.
		// Insert to senderfiles
		key = NFTP_HASH((uint8_t *)fname, strlen(fname));
		strcpy(fullpath, fpath);
		if (ht_contains(&senderfiles, &key)) {
			nftp_log("The last context of file [%s] was covered", fname);
			ht_erase(&senderfiles, &key);
		}
		if (0 != (rv = ht_insert(&senderfiles, &key, fullpath))) {
			nftp_fatal("Error in hash");
			return (NFTP_ERR_HT);
		}
		break;

	case NFTP_TYPE_ACK:
		p->type = NFTP_TYPE_ACK;
		p->len = 6 + 4;
		p->id = 0xff & key;
		p->fileid = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (0 > n) return (NFTP_ERR_ID);
		if (0 != (rv = nftp_file_readblk(fpath, n, (char **)&v, &len))) {
			return rv;
		}

		// Note. No type check.
		p->type = type;

		p->len = 5 + 4 + 2 + 4 + len;
		p->fileid = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));
		p->blockseq = n;

		p->ctlen = len;
		p->content = (uint8_t *)v;
		break;

	case NFTP_TYPE_GIVEME:
		if (0 > n) return (NFTP_ERR_ID);
		p->type = NFTP_TYPE_GIVEME;
		p->len = 5 + 4 + 2;

		p->fileid = NFTP_HASH((const uint8_t *)fname, (size_t)strlen(fname));

		p->blockseq = n;
		break;

	default:
		nftp_fatal("NOT SUPPORTED");
		break;
	}

	size_t alen;
	if (0 != (rv = nftp_encode(p, (uint8_t **)rmsg, &alen))) {
		return rv;
	}
	*rlen = alen;

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
nftp_proto_handler(char *msg, int len, char **rmsg, int *rlen)
{
	int             rv       = 0;
	uint32_t        hashcode = 0;
	nftp *          n;
	struct nctx *   ctx = NULL;
	struct file_cb *fcb = NULL;
	nftp_iter *     iter = NULL;
	char            partname[NFTP_FNAME_LEN + 8];
	char            fullpath[NFTP_FNAME_LEN + NFTP_FDIR_LEN];
	char            fullpath2[NFTP_FNAME_LEN + NFTP_FDIR_LEN];
	size_t          blocks;

	if (0 != (rv = nftp_alloc(&n))) return rv;

	// Set default return value
	*rmsg = NULL;
	*rlen = 0;

	if (0 != (rv = nftp_decode(n, (uint8_t *)msg, len))) {
		nftp_free(n);
		return rv;
	}

	switch (n->type) {
	case NFTP_TYPE_HELLO:
		ctx = nctx_alloc(n->blocks);
		ctx->fileid = NFTP_HASH((const uint8_t *)n->fname,
		        strlen(n->fname));
		ctx->hashcode = n->hashcode;

		if (ht_contains(&files, &n->fileid)) {
			nftp_fatal("File with same fileid is processing [%d][%s]", n->fileid, n->fname);
			nctx_free(ctx);
			nftp_free(n);
			return NFTP_ERR_HT;
		}

		iter = nftp_iter_alloc(NFTP_SCHEMA_VEC, fcb_reg);
		nftp_iter_next(iter);
		while (iter->key != NFTP_TAIL) {
			fcb = iter->val;
			if (0 == strcmp(fcb->fname, n->fname))
				ctx->fcb = fcb;
			nftp_iter_next(iter);
		}
		nftp_iter_free(iter);

		if (NULL == ctx->fcb) {
			nftp_log("Set default callback for file [%s]", n->fname);
			nftp_vec_get(fcb_reg, 0, (void **)&fcb);
			ctx->fcb = fcb;
			nftp_proto_register(n->fname, fcb->cb, fcb->arg);
		}

		nftp_file_fullpath(fullpath, recvdir, n->fname);
		if (nftp_file_exist(fullpath)) {
			nftp_file_newname(n->fname, &ctx->wfname, recvdir);
			nftp_log("File [%s] exists, recver would save to [%s]",
			        n->fname, ctx->wfname);
		} else {
			if ((ctx->wfname = malloc(n->namelen+1)) == NULL) {
				nftp_free(n);
				return (NFTP_ERR_MEM);
			}
			strcpy(ctx->wfname, n->fname);
		}
		nftp_file_partname(partname, ctx->wfname);
		nftp_file_fullpath(fullpath, recvdir, partname);
		if (0 != (rv = nftp_file_write(fullpath, "", 0))) { // create file
			nftp_fatal("File write failed [%s]", fullpath);
			nftp_free(n);
			return rv;
		}

		if (0 != (rv = ht_insert(&files, &ctx->fileid, &ctx))) {
			nftp_fatal("Error in hash");
			nftp_free(n);
			return (NFTP_ERR_HT);
		}
		ctx->status = NFTP_STATUS_HELLO;

		nftp_proto_maker(n->fname, NFTP_TYPE_ACK, n->id, 0, rmsg, rlen);
		break;

	case NFTP_TYPE_ACK:
		// TODO return An Iterator
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (!ht_contains(&files, &n->fileid)) {
			nftp_fatal("Not found fileid [%d]", n->fileid);
			nftp_free(n);
			return NFTP_ERR_HT;
		}
		ctx = *((struct nctx **)ht_lookup(&files, &n->fileid));

		nftp_file_partname(partname, ctx->wfname);
		nftp_file_fullpath(fullpath, recvdir, partname);

		if (n->blockseq == ctx->nextid) {
			rv = nftp_file_append(fullpath, (char *)n->content, n->ctlen);
			if (0 != rv) {
				nftp_fatal("Error in file append [%s]", fullpath);
				nftp_free(n);
				return rv;
			}
			do {
				ctx->nextid ++;
				if ((ctx->nextid > ctx->cap-1) ||
				    (ctx->entries[ctx->nextid].body == NULL))
					break;
				rv = nftp_file_append(fullpath,
				        ctx->entries[ctx->nextid].body,
				        ctx->entries[ctx->nextid].len);
				if (0 != rv) {
					nftp_fatal("Error in file append [%s]", fullpath);
					nftp_free(n);
					return rv;
				}
				free(ctx->entries[ctx->nextid].body);
				ctx->entries[ctx->nextid].body = NULL;
				ctx->entries[ctx->nextid].len  = 0;
			} while (1);
		} else {
			// Just store it
			if (ctx->entries[n->blockseq].len != 0 &&
			    ctx->entries[n->blockseq].body != NULL) {
				free(ctx->entries[n->blockseq].body);
				ctx->len --; // replace rather than add
			}
			ctx->entries[n->blockseq].len = n->ctlen;
			ctx->entries[n->blockseq].body = (char *)n->content;
			n->content = NULL; // avoid be free
		}

		ctx->len ++;
		//nftp_log("Process(recv) [%s]:[%d/%d]",
		//	ctx->wfname, ctx->nextid, ctx->cap);

		if (n->type == NFTP_TYPE_FILE) ctx->status = NFTP_STATUS_TRANSFER;
		if (n->type == NFTP_TYPE_END) ctx->status = NFTP_STATUS_END;

		// Recved finished
		if (ctx->nextid == ctx->cap) {
			ctx->status = NFTP_STATUS_FINISH;
			// Rename
			nftp_file_fullpath(fullpath2, recvdir, ctx->wfname);
			rv = nftp_file_rename(fullpath, fullpath2);
			if (0 != rv) {
				nftp_fatal("Error happened in file rename [%s].", fullpath);
				nftp_free(n);
				return rv;
			}
			*rmsg = strdup(ctx->wfname);
			*rlen = strlen(ctx->wfname);
			// hash check
			rv = nftp_file_hash(fullpath2, &hashcode);
			if (0 != rv) {
				nftp_fatal("Error happened in file hash [%s].", fullpath2);
				nftp_free(n);
				return rv;
			}
			if (ctx->hashcode != hashcode) {
				nftp_log("Hash check failed [%s].", ctx->wfname);
				nftp_free(n);
				return (NFTP_ERR_PROTO);
			} else {
				nftp_log("Hash check passed [%s].", ctx->wfname);
			}

			// Run cb
			if (NULL == ctx->fcb) {
				nftp_log("Unregistered.");
				goto next;
			}

			if (ctx->fcb->cb)
				ctx->fcb->cb(ctx->fcb->arg);

			// Free resource
			for (int i=0; i<nftp_vec_len(fcb_reg); ++i)
				if (0 == nftp_vec_get(fcb_reg, i, (void **)&fcb))
					if (ctx->fcb == fcb) {
						if (i == 0)
							goto next;
						nftp_vec_delete(fcb_reg, (void **)&fcb, i);
						break;
					}

			free(ctx->fcb->fname);
			free(ctx->fcb);

		next:
			if (0 != (rv = ht_erase(&files, &ctx->fileid))) {
				nftp_fatal("Not find the key [%d] in hashtable.", ctx->fileid);
				nftp_free(n);
				return (NFTP_ERR_HT);
			}
			nctx_free(ctx);
		}
		break;

	case NFTP_TYPE_GIVEME:
		if (!ht_contains(&senderfiles, &n->fileid)) {
			nftp_fatal("Not found fileid [%d][%s]", n->fileid, n->fname);
			nftp_free(n);
			return NFTP_ERR_HT;
		}

		strcpy(fullpath, (char *)ht_lookup(&senderfiles, &n->fileid));

		if ((rv = nftp_file_blocks(fullpath, &blocks)) != 0) {
			nftp_fatal("Error in reading blocks [%s]", fullpath);
			nftp_free(n);
			return rv;
		}

		if (n->blockseq == blocks-1)
			nftp_proto_maker(fullpath, NFTP_TYPE_END, n->fileid, n->blockseq, rmsg, rlen);
		else
			nftp_proto_maker(fullpath, NFTP_TYPE_FILE, n->fileid, n->blockseq, rmsg, rlen);

		break;

	default:
		nftp_fatal("NOT SUPPORTED");
		break;
	}
	nftp_free(n);

	return (0);
}

// nftp_proto_register function is used to determine the files
// to to received. Only the msg with fileid registered would
// be handled. When all the msgs marked with a fileid are
// received, the cb(arg) function would be executed.
int
nftp_proto_register(char * fname, int (*cb)(void *), void *arg)
{
	int rv;

	struct file_cb * fcb, *fcbn;

	if (NULL == fname) return (NFTP_ERR_FILENAME);

	if ((fcb = malloc(sizeof(struct file_cb))) == NULL) {
		return (NFTP_ERR_MEM);
	}
	fcb->cb = cb;
	fcb->arg = arg;

	if ((fcb->fname = malloc(strlen(fname)+1)) == NULL) {
		free(fcb);
		return (NFTP_ERR_MEM);
	}
	strcpy(fcb->fname, fname);

	if (0 == strcmp("*", fname)) {
		// '*' always the first one
		if (nftp_vec_len(fcb_reg) == 0) {
			rv = nftp_vec_push(fcb_reg, fcb, NFTP_TAIL);
			if (rv != 0)
				goto err;
			return (0);
		}
		// Update the cb and arg of '*'
		rv = nftp_vec_get(fcb_reg, 0, (void **)&fcbn);
		if (rv != 0)
			goto err;
		if (0 != strcmp(fcbn->fname, fname)) {
			rv = NFTP_ERR_VEC;
			goto err;
		}
		fcbn->cb = cb;
		fcbn->arg = arg;
		// Free
		free(fcb->fname);
		free(fcb);
	} else {
		// Not allowed two files with same filename are processing
		for (int i=1; i<nftp_vec_len(fcb_reg); ++i) {
			rv = nftp_vec_get(fcb_reg, i, (void **)&fcbn);
			if (rv != 0)
				goto err;
			if (0 == strcmp(fcbn->fname, fcb->fname)) {
				nftp_fatal("File (%s) is processing...", fcb->fname);
				rv = NFTP_ERR_HT;
				goto err;
			}
		}
		rv = nftp_vec_push(fcb_reg, fcb, NFTP_TAIL);
		if (rv != 0)
			goto err;
	}

	return (0);

err:
	free(fcb->fname);
	free(fcb);
	return rv;
}

int
nftp_proto_unregister(char * fname)
{
	int rv;

	struct file_cb *fcb;

	if (NULL == fname) return (NFTP_ERR_FILENAME);

	if (0 == strcmp("*", fname))
		// Not allowed. I think...
		return NFTP_ERR_FILENAME;

	// Find the fcb with this name and delete it
	for (int i=1; i<nftp_vec_len(fcb_reg); ++i) {
		rv = nftp_vec_get(fcb_reg, i, (void **)&fcb);
		if (rv != 0)
			return NFTP_ERR_VEC;
		if (0 == strcmp(fcb->fname, fname)) {
			nftp_vec_delete(fcb_reg, (void **)&fcb, i);
			free(fcb->fname);
			free(fcb);
			return (0);
		}
	}

	return (NFTP_ERR_HT);
}

int
nftp_set_recvdir(char * dir)
{
	char * rdir;
	if ((rdir = malloc(strlen(dir)+1)) == NULL) {
		return (NFTP_ERR_MEM);
	}
	if (recvdir) {
		free(recvdir);
		recvdir = NULL;
	}

	strcpy(rdir, dir);
	recvdir = rdir;

	return (0);
}

int
test()
{
	printf("Hello World\n");
	return 0;
}
