// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <pthread.h>
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

struct _iovs {
	struct iovec *  iovs;
	size_t          low; // iov loaded from iovs+low
	size_t          len; // counter of iov in iovs
	size_t          cap;
	size_t          iolen;
	pthread_mutex_t mtx;
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

	pthread_mutex_init(&iovs->mtx, NULL);

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

	pthread_mutex_lock(&iovs->mtx);

	for (int i = iovs->low + iovs->len; i > iovs->low + pos; --i) {
		iovs->iovs[i].iov_base = iovs->iovs[i - 1].iov_base;
		iovs->iovs[i].iov_len  = iovs->iovs[i - 1].iov_len;
	}
	iovs->iovs[pos].iov_base = ptr;
	iovs->iovs[pos].iov_len  = len;

	iovs->len ++;
	iovs->iolen += len;

	pthread_mutex_unlock(&iovs->mtx);
	return (0);
}

int
nftp_iovs_push(nftp_iovs *iovs, void *ptr, size_t len, int flag)
{
	struct iovec *iov;

	pthread_mutex_lock(&iovs->mtx);
	if (flag == NFTP_HEAD) {
		if (iovs->low == 0) {
			pthread_mutex_unlock(&iovs->mtx);
			return (NFTP_ERR_OVERFLOW); // Resize TODO
		}

		iovs->low--;
		iov = &iovs->iovs[iovs->low];
	} else if (flag == NFTP_TAIL) {
		if (iovs->low + iovs->len > iovs->cap) {
			pthread_mutex_unlock(&iovs->mtx);
			return (NFTP_ERR_OVERFLOW); // Resize TODO
		}

		iov = &iovs->iovs[iovs->low + iovs->len];
	} else {
		pthread_mutex_unlock(&iovs->mtx);
		return (NFTP_ERR_FLAG);
	}
	iov->iov_base = ptr;
	iov->iov_len  = len;

	iovs->len++;
	iovs->iolen += len;

	pthread_mutex_unlock(&iovs->mtx);
	return (0);
}

int
nftp_iovs_pop(nftp_iovs *iovs, void **ptrp, size_t *lenp, int flag)
{
	struct iovec *iov;

	if (iovs->len == 0) {
		return (NFTP_ERR_EMPTY);
	}

	pthread_mutex_lock(&iovs->mtx);

	if (flag == NFTP_HEAD) {
		iov = &iovs->iovs[iovs->low];
		iovs->low++;
	} else if (flag == NFTP_TAIL) {
		iov = &iovs->iovs[iovs->low + iovs->len - 1];
	} else {
		pthread_mutex_unlock(&iovs->mtx);
		return (NFTP_ERR_FLAG);
	}

	*ptrp         = iov->iov_base;
	*lenp         = iov->iov_len;
	iov->iov_base = NULL;
	iov->iov_len  = 0;

	iovs->len--;
	iovs->iolen -= (*lenp);

	pthread_mutex_unlock(&iovs->mtx);
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

	pthread_mutex_lock(&d->mtx);
	pthread_mutex_lock(&s->mtx);

	int idx = d->low + d->len;
	for (int i = 0; i < s->len; ++i) {
		d->iovs[idx + i].iov_base = s->iovs[s->low + i].iov_base;
		d->iovs[idx + i].iov_len  = s->iovs[s->low + i].iov_len;
	}
	pthread_mutex_unlock(&s->mtx);

	d->len += s->len;
	d->iolen += s->iolen;

	pthread_mutex_unlock(&d->mtx);
	return (0);
}

int
nftp_iovs_free(nftp_iovs *iovs)
{
	if (iovs == NULL || iovs->iovs == NULL) {
		return (NFTP_ERR_MEM);
	}

	pthread_mutex_destroy(&iovs->mtx);
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

	pthread_mutex_lock(&iovs->mtx);

	for (int i=iovs->low; i<iovs->low + iovs->len; ++i) {
		memcpy(str + pos, iovs->iovs[i].iov_base, iovs->iovs[i].iov_len);
		pos += iovs->iovs[i].iov_len;
	}

	pthread_mutex_unlock(&iovs->mtx);

	*strp = str;
	*len = iovs->iolen;
	return 0;
}

// Implementation of iterator for iovs
static nftp_iter *
iovs_iter_next(nftp_iter *self)
{
	nftp_iovs * iovs = self->matrix;

	self->key ++;
	self->val = NULL;
	if (self->key <= iovs->len)
		self->val = &iovs->iovs[iovs->low + self->key];

	return self;
}

static nftp_iter *
iovs_iter_prev(nftp_iter *self)
{
	nftp_iovs * iovs = self->matrix;

	self->key --;
	self->val = NULL;
	if (self->key <= iovs->len)
		self->val = &iovs->iovs[iovs->low + self->key];

	return self;
}

static void
iovs_iter_free(nftp_iter *self)
{
	free(self);
}

nftp_iter *
nftp_iovs_iter(nftp_iovs *iovs)
{
	nftp_iter * iter;

	if (NULL == (iter = malloc(sizeof(*iter))))
		return NULL;

	iter->next = iovs_iter_next;
	iter->prev = iovs_iter_prev;
	iter->free = iovs_iter_free;
	iter->key = 0;
	iter->val = NULL;
	iter->matrix = (void *)iovs;

	return iter;
}

