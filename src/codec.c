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
	p->filename = NULL;
	p->namelen = 0;
	p->fileflag = 0;
	p->hashcode = 0;
	p->content = 0;
	p->ctlen = 0;
	if ((p->exbuf = malloc(sizeof(char) * 10)) == NULL) {
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
	p->id = *(v + pos); ++pos; // id

	// Check if iolen eq to the length from decoding
	if (len != p->len) return (NFTP_ERR_STREAM);

	// Option parameter
	switch ((uint32_t)p->type) {
	case NFTP_TYPE_HELLO:
		if (p->id != 0) return (NFTP_ERR_ID);
		p->blocks = (int) *(v + pos); ++pos;
		nftp_get_u16(v + pos, p->namelen); pos += 2;

		if ((p->filename = malloc(sizeof(char) * (1 + p->namelen))) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->filename, v + pos, p->namelen); pos += p->namelen;
		p->filename[p->namelen] = '\0';

		p->ctlen = p->len - pos;
		if ((p->content = malloc(sizeof(char) * p->ctlen)) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;
		nftp_get_u32(p->content, p->hashcode);
		break;

	case NFTP_TYPE_ACK:
		if (p->id != 0) return (NFTP_ERR_ID);
		nftp_get_u32(v + pos, p->fileflag); pos += 4;
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (p->id == 0) return (NFTP_ERR_ID);
		nftp_get_u32(v + pos, p->fileflag); pos += 4;

		p->ctlen = p->len - pos - 1;
		if ((p->content = malloc(sizeof(char) * p->ctlen)) == NULL)
			return (NFTP_ERR_MEM);
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;

		p->crc = *(v + pos - 1);
		break;

	case NFTP_TYPE_GIVEME: // TODO
		fatal("NOT SUPPORTED");
		return (NFTP_ERR_PROTO);
	default:
		fatal("UNDEFINED TYPE PACKET");
		return (NFTP_ERR_PROTO);
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
		if (0 != nftp_iovs_append(iovs, (void *)&p->blocks, 1)) goto error;

		nftp_put_u16(p->exbuf + 4, p->namelen);
		if (0 != nftp_iovs_append(iovs, (void *)p->exbuf + 4, 2) ||
		    0 != nftp_iovs_append(iovs, (void *)p->filename, p->namelen)) {
			goto error;
		}

		nftp_put_u32(p->exbuf + 6, p->hashcode);
		rv |= nftp_iovs_append(iovs, (void *)p->exbuf + 6, 4);
		break;
		
	case NFTP_TYPE_ACK:
		nftp_put_u32(p->exbuf + 4, p->fileflag);
		rv |= nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 4);
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (p->id == 0) return (NFTP_ERR_ID);
		nftp_put_u32(p->exbuf + 4, p->fileflag);
		if (0 != nftp_iovs_append(iovs, (void *)(p->exbuf + 4), 4) ||
		    0 != nftp_iovs_append(iovs, (void *)p->content, p->ctlen)) {
			goto error;
		}

		p->crc = nftp_crc(p->content, p->ctlen);
		rv |= nftp_iovs_append(iovs, (void *)&p->crc, 1);
		break;

	case NFTP_TYPE_GIVEME:
		fatal("NOT SUPPORTED");
		return (NFTP_ERR_PROTO);
	default:
		fatal("UNDEFINED TYPE PACKET");
		return (NFTP_ERR_PROTO);
	}
	if (0 != rv) goto error;

	nftp_put_u32(p->exbuf, p->len);
	if (0 != nftp_iovs_push(iovs, (void *)&p->id, 1, NFTP_HEAD)   ||
	    0 != nftp_iovs_push(iovs, (void *)p->exbuf, 4, NFTP_HEAD) ||
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

	if (p->filename) {
		free(p->filename);
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

