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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "nftp.h"

int
nftp_proto_alloc(nftp ** pp)
{
	nftp * p;

	if ((p = malloc(sizeof(nftp))) == NULL) {
		return NFTP_ERR_MEM);
	}

	p->type = 0x00;
	p->id = 0;
	p->blocks = 0;
	p->filename = NULL;
	p->namelen = 0;
	p->fileflag = 0;
	p->content = 0;
	p->ctlen = 0;

	*pp = p;
	return (0);
}

int
nftp_proto_decode_iovs(nftp * p, nftp_iovs * iovs, size_t len)
{
	uint8_t * v;
	int iolen;

	if (!p || !iovs || len == 0) {
		return (NFTP_ERR_EMPTY);
	}

	nftp_iovs2stream(iovs, &v, &iolen);
	nftp_proto_decode(p, v);

	return (0);
}

int
nftp_proto_decode(nftp *p, uint8_t *v)
{
	size_t pos = 0;

	if (!p || !v) {
		return (NFTP_ERR_EMPTY);
	}

	p->type = *(v + pos); ++pos; // type
	nftp_get_u32(v + pos, p->len); pos += 4; // len
	p->id = *(v + pos); ++pos; // id

	// Option parameter
	switch (p->type) {
	case NFTP_TYPE_HELLO:
		if (p->id != 0) return (NFTP_ERR_ID);
		p->blocks = (int) *(v + pos); ++pos;
		p->namelen = nftp_get_u16(v + pos); pos += 2;

		p->filename = malloc(sizeof(char) * (1 + p->namelen));
		memcpy(p->filename, v + pos, p->namelen); pos += p->namelen;
		p->filename[p->namelen] = '\0';

		p->ctlen = p->len - pos - 1;
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;
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
		memcpy(p->content, v + pos, p->ctlen); pos = p->len;
		p->crc = *(v + pos);
		break;

	case NFTP_TYPE_GIVEME: // TODO
		fatal("NOT SUPPORTED");
		break;
	default:
		fatal("UNDEFINED TYPE PACKET");
		break;
	}
	return (0);
}

int
nftp_proto_encode_iovs(nftp * p, nftp_iovs * iovs)
{
	uint8_t buf[2];

	if (!p || !iovs) {
		return (NFTP_ERR_EMPTY);
	}
	if (nftp_iovs_len(iovs) != 0) {
		return (NFTP_ERR_DIRTY);
	}
	// Type & len & Id will push in the end

	switch (p->type) {
	case NFTP_TYPE_HELLO:
		nftp_iovs_append(iovs, blocks, 1);

		nftp_put_u16(buf, p->namelen);
		nftp_iovs_append(iovs, buf, 2);
		nftp_iovs_append(iovs, p->filename, p->namelen);

		nftp_iovs_append(iovs, p->content, 4);
		break;
		
	case NFTP_TYPE_ACK:
		nftp_iovs_append(iovs, p->fileflag, 4);
		break;

	case NFTP_TYPE_FILE:
	case NFTP_TYPE_END:
		if (p->id == 0) return (NFTP_ERR_ID);
		nftp_iovs_append(iovs, p->fileflag, 4);
		nftp_iovs_append(iovs, p->content, p->ctlen);

		buf[0] = nftp_crc(p->content, p->ctlen);
		nftp_iovs_append(iovs, buf, 1);
		break;

	case NFTP_TYPE_GIVEME:
		fatal("NOT SUPPORTED");
		break;
	default:
		fatal("UNDEFINED TYPE PACKET");
		break;
	}

	nftp_iovs_push(iovs, p->id, 1, NFTP_HEAD);
	nftp_iovs_append(iovs, p->len, 4, NFTP_HEAD);
	nftp_iovs_append(iovs, p->type, 1, NFTP_HEAD);
	return (0);
}

int
nftp_proto_encode(nftp * p, uint8_t ** vp, size_t * len)
{
	uint8_t * v;
	nftp_iovs * iovs;

	nftp_iovs_alloc(&iovs);
	nftp_proto_encode(p, iovs);
	nftp_iovs2stream(iovs, &v, len);

	**vp = v;
	return (0);
}

int
nftp_proto_free(nftp * p)
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

	free(p);
	return (0);
}

