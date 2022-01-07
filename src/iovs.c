// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
struct iovec {
	void * iov_base;
	size_t iov_len;
};
#else
#include <sys/uio.h>
#endif

#include "nftp.h"

struct nftp_iovs {
	struct iovec *iovs;
	size_t        low; // iov loaded from iovs+low
	size_t        len; // counter of iov in iovs
	size_t        cap;
	size_t        iolen;
};

static int
resize(nftp_iovs *iovs, size_t sz)
{
	return (0);
}

int
nftp_iovs_alloc(nftp_iovs **iovsp)
{
	nftp_iovs *iovs;

	if ((iovs = malloc(sizeof(nftp_iovs))) == NULL) {
		return (NFTP_ERR_MEM);
	}

	if ((iovs->iovs = malloc(sizeof(struct iovec) * NFTP_SIZE)) == NULL) {
		free(iovs);
		return (NFTP_ERR_MEM);
	}

	iovs->cap = NFTP_SIZE;
	iovs->len = 0;
	iovs->low = iovs->cap / 3;
	iovs->iolen = 0;

	*iovsp = iovs;
	return (0);
}

int
nftp_iovs_append(nftp_iovs *iovs, void *ptr, size_t len)
{
	return nftp_iovs_push(iovs, ptr, len, NFTP_TAIL);
}

// Insert operation is not efficient, using sparingly.
int
nftp_iovs_insert(nftp_iovs *iovs, void *ptr, size_t len, size_t pos)
{
	if (iovs->low + iovs->len + 1 > iovs->cap) {
		return (NFTP_ERR_OVERFLOW); // Resize TODO
	}

	if (pos > iovs->len) {
		return nftp_iovs_push(iovs, ptr, len, NFTP_TAIL);
	}

	for (int i = iovs->low + iovs->len; i > iovs->low + pos; --i) {
		iovs->iovs[i].iov_base = iovs->iovs[i - 1].iov_base;
		iovs->iovs[i].iov_len  = iovs->iovs[i - 1].iov_len;
	}
	iovs->iovs[pos].iov_base = ptr;
	iovs->iovs[pos].iov_len  = len;

	iovs->iolen += len;
	return (0);
}

int
nftp_iovs_push(nftp_iovs *iovs, void *ptr, size_t len, int flag)
{
	struct iovec *iov;

	if (flag == NFTP_HEAD) {
		if (iovs->low == 0) {
			return (NFTP_ERR_OVERFLOW); // Resize TODO
		}

		iovs->low--;
		iov = &iovs->iovs[iovs->low];
	} else if (flag == NFTP_TAIL) {
		if (iovs->low + iovs->len > iovs->cap) {
			return (NFTP_ERR_OVERFLOW); // Resize TODO
		}

		iov = &iovs->iovs[iovs->low + iovs->len];
	}
	iov->iov_base = ptr;
	iov->iov_len  = len;

	iovs->len++;
	iovs->iolen += len;
	return (0);
}

int
nftp_iovs_pop(nftp_iovs *iovs, void **ptrp, size_t *lenp, int flag)
{
	struct iovec *iov;

	if (iovs->len == 0) {
		return (NFTP_ERR_EMPTY);
	}

	if (flag == NFTP_HEAD) {
		iov = &iovs->iovs[iovs->low];
		iovs->low++;
	} else if (flag == NFTP_TAIL) {
		iov = &iovs->iovs[iovs->low + iovs->len - 1];
	}

	*ptrp         = iov->iov_base;
	*lenp         = iov->iov_len;
	iov->iov_base = NULL;
	iov->iov_len  = 0;

	iovs->len--;
	iovs->iolen -= (*lenp);
	return 0;
}

size_t
nftp_iovs_len(nftp_iovs *iovs)
{
	return iovs->len;
}

size_t
nftp_iovs_cap(nftp_iovs *iovs)
{
	return iovs->cap;
}

int
nftp_iovs_cat(nftp_iovs *dest, nftp_iovs *src)
{
	nftp_iovs *d = dest, *s = src;

	if (d->low + d->len + s->len > d->cap) {
		return (NFTP_ERR_OVERFLOW); // Resize TODO
	}

	int idx = d->low + d->len;
	for (int i = 0; i < s->len; ++i) {
		d->iovs[idx + i].iov_base = s->iovs[s->low + i].iov_base;
		d->iovs[idx + i].iov_len  = s->iovs[s->low + i].iov_len;
	}

	d->len += s->len;
	d->iolen += s->iolen;
	return (0);
}

int
nftp_iovs_free(nftp_iovs *iovs)
{
	if (iovs == NULL || iovs->iovs == NULL) {
		return (NFTP_ERR_MEM);
	}

	free(iovs->iovs);
	free(iovs);
	return (0);
}

int
nftp_iovs2stream(nftp_iovs *iovs, uint8_t **strp, size_t *len)
{
	size_t pos = 0;
	uint8_t * str;
	if ((str= malloc(iovs->iolen)) == NULL) {
		return (NFTP_ERR_MEM);
	}

	for (int i=iovs->low; i<iovs->low + iovs->len; ++i) {
		memcpy(str + pos, iovs->iovs[i].iov_base, iovs->iovs[i].iov_len);
		pos += iovs->iovs[i].iov_len;
	}

	*strp = str;
	*len = iovs->iolen;
	return 0;
}

