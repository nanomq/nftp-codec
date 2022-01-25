// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <assert.h>

#include "nftp.h"

int
test_vector()
{
	nftp_log("test_vector");
	nftp_vec *v1, *v2;
	char *e0 = "e0", *e1, *e2 = "e2";
	int idx;

	assert(0 == nftp_vec_alloc(&v1));
	assert(NULL != v1);
	assert(0 == nftp_vec_len(v1));
	assert(NFTP_SIZE == nftp_vec_cap(v1));

	assert(0 == nftp_vec_append(v1, (void *)e0));
	assert(1 == nftp_vec_len(v1));
	assert(0 == nftp_vec_get(v1, 0, (void **)&e1));
	assert(e1 == e0);

	assert(0 == nftp_vec_insert(v1, (void *)e2, 1));
	assert(2 == nftp_vec_len(v1));
	assert(0 == nftp_vec_delete(v1, (void **)&e1, 1));
	assert(1 == nftp_vec_len(v1));
	assert(e2 == e1);
	assert(0 == nftp_vec_delete(v1, (void **)&e1, 0));
	assert(0 == nftp_vec_len(v1));
	assert(e0 == e1);

	assert(0 == nftp_vec_push(v1, (void *)e0, NFTP_HEAD));
	assert(1 == nftp_vec_len(v1));
	assert(0 == nftp_vec_push(v1, (void *)e2, NFTP_HEAD));
	assert(2 == nftp_vec_len(v1));
	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_TAIL));
	assert(e0 == e1);
	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_TAIL));
	assert(e2 == e1);
	assert(0 == nftp_vec_len(v1));

	assert(0 == nftp_vec_push(v1, (void *)e0, NFTP_TAIL));
	assert(0 == nftp_vec_push(v1, (void *)e2, NFTP_TAIL));
	assert(2 == nftp_vec_len(v1));

	assert(0 == nftp_vec_getidx(v1, (void *)e2, &idx));
	assert(1 == idx);

	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_HEAD));
	assert(e0 == e1);
	assert(1 == nftp_vec_len(v1));

	assert(0 == nftp_vec_alloc(&v2));
	assert(0 == nftp_vec_len(v2));
	assert(0 == nftp_vec_append(v2, (void *)e2));
	assert(0 == nftp_vec_append(v2, (void *)e2));
	assert(0 == nftp_vec_append(v2, (void *)e2));
	assert(3 == nftp_vec_len(v2));

	assert(0 == nftp_vec_cat(v1, v2));
	assert(4 == nftp_vec_len(v1));

	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_HEAD));
	assert(e2 == e1);
	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_HEAD));
	assert(e2 == e1);
	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_HEAD));
	assert(e2 == e1);
	assert(0 == nftp_vec_pop(v1, (void **)&e1, NFTP_HEAD));
	assert(e2 == e1);

	assert(0 == nftp_vec_len(v1));

	assert(0 == nftp_vec_free(v1));
	assert(0 == nftp_vec_free(v2));
}

