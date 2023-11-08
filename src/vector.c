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

#include "nftp.h"

struct _vec {
	int             cap; // capicity
	int             len; // number of elements
	int             low; // elements stored from here
	void          **vec;
	pthread_mutex_t mtx;
};

/*
static int
resize(nftp_vec *v) // TODO
{
	(void) v;
	return (0);
}
*/

int
nftp_vec_alloc(nftp_vec **vp, int sz)
{
	nftp_vec *v;

	if ((v = malloc(sizeof(*v))) == NULL)
		return (NFTP_ERR_MEM);
	if (sz <= 0)
		sz = NFTP_SIZE;
	if ((v->vec = malloc(sizeof(void *) * sz)) == NULL)
		return (NFTP_ERR_MEM);
	pthread_mutex_init(&v->mtx, NULL);

	v->len = 0;
	v->cap = sz;
	v->low = v->cap/4;

	*vp = v;
	return (0);
}

int
nftp_vec_free(nftp_vec *v)
{
	if (!v) return (NFTP_ERR_VEC);
	if (v->vec) free(v->vec);
	pthread_mutex_destroy(&v->mtx);
	free(v);
	return (0);
}

int
nftp_vec_append(nftp_vec *v, void *entry)
{
	return nftp_vec_push(v, entry, NFTP_TAIL);
}

int
nftp_vec_insert(nftp_vec *v, void *entry, int pos)
{
	if (!v) return (NFTP_ERR_VEC);

	if (v->len == v->cap)
		return (NFTP_ERR_OVERFLOW);

	if (pos > v->len)
		return nftp_vec_push(v, entry, NFTP_TAIL);

	pthread_mutex_lock(&v->mtx);
	for (int i = v->low + v->len; i > v->low + pos; --i)
		v->vec[i % v->cap] = v->vec[(i-1) % v->cap];
	v->vec[(v->low + pos) % v->cap] = entry;
	// nftp_log("after insert v[%d]=%p(%c)", (v->low + pos) % v->cap, entry, *(((char*)entry) + 1));

	v->len ++;
	pthread_mutex_unlock(&v->mtx);

	return (0);
}

int
nftp_vec_delete(nftp_vec *v, void **entryp, int pos)
{
	if (!v) return (NFTP_ERR_VEC);

	if (pos < 0 || pos > v->len-1)
		return (NFTP_ERR_OVERFLOW);

	pthread_mutex_lock(&v->mtx);
	*entryp = v->vec[(v->low + pos) % v->cap];
	for (int i = v->low + pos + 1; i < v->low + v->len; ++i) {
		v->vec[(i-1) % v->cap] = v->vec[i % v->cap];
	}
	v->len --;
	pthread_mutex_unlock(&v->mtx);

	return (0);
}

int
nftp_vec_push(nftp_vec *v, void *entry, int flag)
{
	int pos = 0;

	if (!v) return (NFTP_ERR_VEC);

	pthread_mutex_lock(&v->mtx);
	if (v->len == v->cap) {
		pthread_mutex_unlock(&v->mtx);
		return (NFTP_ERR_OVERFLOW);
	}

	if (NFTP_HEAD == flag) {
		v->low --;
		v->low += v->cap;
		v->low %= v->cap;
		pos = v->low;
	} else if (NFTP_TAIL == flag) {
		pos = v->low + v->len;
		pos %= v->cap;
	} else {
		pthread_mutex_unlock(&v->mtx);
		return (NFTP_ERR_FLAG);
	}
	v->len ++;
	// nftp_log("push pos %d", pos);
	v->vec[pos] = entry;
	pthread_mutex_unlock(&v->mtx);

	return (0);
}

int
nftp_vec_pop(nftp_vec *v, void **entryp, int flag)
{
	int pos = 0;

	if (!v) return (NFTP_ERR_VEC);

	if (v->len == 0) return (NFTP_ERR_EMPTY);

	pthread_mutex_lock(&v->mtx);
	if (NFTP_HEAD == flag) {
		pos = v->low;
		v->low ++;
		v->low %= v->cap;
	} else if (NFTP_TAIL == flag) {
		pos = v->low + v->len - 1;
		pos %= v->cap;
	} else {
		pthread_mutex_unlock(&v->mtx);
		return (NFTP_ERR_FLAG);
	}

	*entryp = v->vec[pos];
	v->vec[pos] = NULL;
	// nftp_log("pop pos %d", pos);

	v->len --;
	pthread_mutex_unlock(&v->mtx);

	return (0);
}

int
nftp_vec_get(nftp_vec *v, int idx, void **entryp)
{
	if (!v) return (NFTP_ERR_VEC);
	if (idx >= v->len) return (NFTP_ERR_OVERFLOW);
	if (idx < 0) return (NFTP_ERR_OVERFLOW);

	*entryp = v->vec[(v->low + idx) % v->cap];

	return (0);
}

int
nftp_vec_getidx(nftp_vec *v, void *entry, int *idxp)
{
	if (!v) return (NFTP_ERR_VEC);
	for (int i = v->low; i < v->low + v->len; ++i)
		if (v->vec[i] == entry) {
			*idxp = i - v->low;
			return (0);
		}
	return (NFTP_ERR_EMPTY);
}

int
nftp_vec_cat(nftp_vec *dv, nftp_vec *sv)
{
	if (!dv) return (NFTP_ERR_VEC);
	if (!sv) return (0); // Do not change the destination vector

	if (dv->low + dv->len + sv->len > dv->cap)
		return (NFTP_ERR_OVERFLOW);

	pthread_mutex_lock(&dv->mtx);
	pthread_mutex_lock(&sv->mtx);

	int idx = dv->low + dv->len;
	for (int i = 0; i < sv->len; ++i)
		dv->vec[idx + i] = sv->vec[sv->low + i];

	dv->len += sv->len;

	pthread_mutex_unlock(&sv->mtx);
	pthread_mutex_unlock(&dv->mtx);

	return (0);
}

int
nftp_vec_cap(nftp_vec *v)
{
	return v->cap;
}

int
nftp_vec_len(nftp_vec *v)
{
	return v->len;
}

// Implementation of iterator for vector
static nftp_iter *
vec_iter_next(nftp_iter *self)
{
	nftp_vec * v = self->matrix;

	self->val = NULL;
	if (self->key == NFTP_TAIL) {
	} else {
		self->key ++;
		if (0 != nftp_vec_get(v, self->key, &self->val))
			self->key = NFTP_TAIL;
	}

	return self;
}

static nftp_iter *
vec_iter_prev(nftp_iter *self)
{
	nftp_vec * v = self->matrix;

	self->val = NULL;
	if (self->key == NFTP_TAIL) {
	} else {
		self->key --;
		if (0 != nftp_vec_get(v, self->key, &self->val))
			self->key = NFTP_HEAD;
	}

	return self;
}

static void
vec_iter_free(nftp_iter *self)
{
	free(self);
}

nftp_iter *
nftp_vec_iter(nftp_vec *v)
{
	nftp_iter * iter;

	if (NULL == (iter = malloc(sizeof(*iter))))
		return NULL;

	iter->next = vec_iter_next;
	iter->prev = vec_iter_prev;
	iter->free = vec_iter_free;
	iter->key = NFTP_HEAD;
	iter->val = NULL;
	iter->matrix = (void *)v;

	return iter;
}

