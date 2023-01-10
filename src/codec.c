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

int
nftp_alloc(nftp ** pp)
{
	nftp * p;

	if ((p = malloc(sizeof(nftp))) == NULL) {
		return (NFTP_ERR_MEM);
	}

	p->type = 0x00;
	p->id = 0;
	p->blocks = 0;
	p->fpath = NULL;
	p->fname = NULL;
	p->namelen = 0;
	p->fileid = 0;
	p->hashcode = 0;
	p->content = 0;
	p->ctlen = 0;
	if ((p->exbuf = malloc(sizeof(char) * 12)) == NULL) {
		return (NFTP_ERR_MEM);
	}

	*pp = p;
	return (0);
}

int
nftp_decode_iovs(nftp * p, nftp_iovs * iovs)
{
	uint8_t * v;
	size_t iolen;

	if (!p || !iovs) {
		return (NFTP_ERR_EMPTY);
	}

	if (0 != nftp_iovs2stream(iovs, &v, &iolen)) {
		return (NFTP_ERR_IOVS);
	}

	return nftp_decode(p, v, iolen);
}

int
nftp_decode(nftp *p, uint8_t *v, size_t len)
{
	size_t pos = 0;

	if (!p || !v || !len) return (NFTP_ERR_EMPTY);
	// Ensure the length of stream is longger than fixed header
	if (len < 6) return (NFTP_ERR_STREAM);

	p->type = *(v + pos); ++pos; // type
	nftp_get_u32(v + pos, p->len); pos += 4; // len

	// Check if iolen eq to the length from decoding
	if (len != p->len) return (NFTP_ERR_STREAM);

	// Option parameter
	switch ((uint32_t)p->type) {
	case NFTP_TYPE_HELLO:
		p->id = *(v + pos); ++pos; // id

		nftp_get_u16(v + pos, p->blocks); pos += 2;
		nftp_get_u16(v + pos, p->namelen); pos += 2;

		if ((p->fname = malloc(sizeof(char) * (1 + p->namelen))) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->fname, v + pos, p->namelen); pos += p->namelen;
		p->fname[p->namelen] = '\0';

		p->ctlen = p->len - pos;
		if ((p->content = malloc(sizeof(char) * p->ctlen)) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;
		nftp_get_u32(p->content, p->hashcode);
		break;

	case NFTP_TYPE_ACK:
		p->id = *(v + pos); ++pos; // id

		nftp_get_u32(v + pos, p->fileid); pos += 4;
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		nftp_get_u32(v + pos, p->fileid); pos += 4;

		nftp_get_u16(v + pos, p->blockseq); pos += 2;

		nftp_get_u16(v + pos, p->ctlen); pos += 2;

		if ((p->content = malloc(sizeof(char) * p->ctlen)) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;
		break;

	case NFTP_TYPE_GIVEME:
		p->id = *(v + pos); pos += 1;
		if (p->id == 0) return (NFTP_ERR_ID);
		nftp_get_u16(v + pos, p->namelen); pos += 2;

		if ((p->fname = malloc(sizeof(char) * (1 + p->namelen))) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->fname, v + pos, p->namelen); pos += p->namelen;
		p->fname[p->namelen] = '\0';
		break;

	default:
		return (NFTP_ERR_TYPE);
	}
	return (0);
}

int
nftp_encode_iovs(nftp * p, nftp_iovs * iovs)
{
	int rv = 0;

	if (!p || !iovs) return (NFTP_ERR_EMPTY);
	if (nftp_iovs_len(iovs) != 0) return (NFTP_ERR_IOVS); // Dirty Iovs
	// Type & len & Id will push in the end

	switch (p->type) {
	case NFTP_TYPE_HELLO:
		if (0 != nftp_iovs_append(iovs, (void *)&p->id, 1)) goto error;

		nftp_put_u16(p->exbuf + 4, p->blocks);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 2)) goto error;

		nftp_put_u16(p->exbuf + 6, p->namelen);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 6), 2) ||
		    0 != nftp_iovs_append(iovs, (void *)p->fname, p->namelen)) {
			goto error;
		}

		nftp_put_u32(p->exbuf + 8, p->hashcode);
		rv |= nftp_iovs_append(iovs, (void *)(p->exbuf + 8), 4);
		break;

	case NFTP_TYPE_ACK:
		if (0 != nftp_iovs_append(iovs, (void *)&p->id, 1)) goto error;
		nftp_put_u32(p->exbuf + 4, p->fileid);
		rv |= nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 4);
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		nftp_put_u32(p->exbuf + 4, p->fileid);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 4))
			goto error;

		nftp_put_u16(p->exbuf + 8, p->blockseq);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 8), 2))
			goto error;

		nftp_put_u16(p->exbuf + 10, p->ctlen);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 10), 2))
			goto error;

		if (0 != nftp_iovs_append(iovs, (void *)p->content, p->ctlen))
			goto error;
		break;

	case NFTP_TYPE_GIVEME:
		if (p->id == 0) return (NFTP_ERR_ID);

		nftp_put_u16(p->exbuf + 4, p->namelen);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 2) ||
		    0 != nftp_iovs_append(iovs, (void *)p->fname, p->namelen)) {
			goto error;
		}
		break;

	default:
		return (NFTP_ERR_TYPE);
	}
	if (0 != rv) goto error;

	nftp_put_u32(p->exbuf, p->len);
	if (0 != nftp_iovs_push(iovs, (void *)p->exbuf, 4, NFTP_HEAD) ||
	    0 != nftp_iovs_push(iovs, (void *)&p->type, 1, NFTP_HEAD)) {
		goto error;
	}

	return (0);

error:
	return (NFTP_ERR_IOVS);
}

int
nftp_encode(nftp * p, uint8_t ** vp, size_t * len)
{
	uint8_t * v;
	nftp_iovs * iovs;
	int rv = 0;

	if ((rv = nftp_iovs_alloc(&iovs)) != 0) {
		return rv;
	}
	if ((rv = nftp_encode_iovs(p, iovs)) != 0) {
		return rv;
	}
	if ((rv = nftp_iovs2stream(iovs, &v, len)) != 0) {
		return rv;
	}

	nftp_iovs_free(iovs);

	*vp = v;
	return (0);
}

int
nftp_free(nftp * p)
{
	if (p == NULL) {
		return (NFTP_ERR_EMPTY);
	}

	if (p->fpath) {
		free(p->fpath);
	}
	if (p->fname) {
		free(p->fname);
	}
	if (p->content) {
		free(p->content);
	}
	if (p->exbuf) {
		free(p->exbuf);
	}

	free(p);
	return (0);
}

