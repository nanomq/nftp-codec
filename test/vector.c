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
#include "test.h"

int
test_vector()
{
	nftp_log("test_vector");
	nftp_vec *v1;
	char *e, *e0 = "e0", *e1 = "e1", *e2 = "e2";
	int idx;
	int cap = NFTP_SIZE / 4;

	assert(0 == nftp_vec_alloc(&v1, cap));
	assert(NULL != v1);
	assert(0 == nftp_vec_len(v1));
	assert(cap == nftp_vec_cap(v1));

	assert(0 == nftp_vec_append(v1, (void *)e0));
	assert(1 == nftp_vec_len(v1));
	assert(0 == nftp_vec_get(v1, 0, (void **)&e));
	assert(e == e0);

	char *earr[3] = {e0, e1, e2};

	// Push test
	int k=0;
	for (int i=0; i<cap - 1; ++i) {
		k ++;
		assert(0 == nftp_vec_push(v1, (void *) earr[k % 3], NFTP_TAIL));
		assert(i+2 == nftp_vec_len(v1));
	}

	assert(NFTP_ERR_OVERFLOW == nftp_vec_push(v1, (void *) earr[0], NFTP_TAIL));
	assert(NFTP_ERR_OVERFLOW == nftp_vec_push(v1, (void *) earr[0], NFTP_HEAD));

	// Pop test
	k=0;
	for (int i=0; i<cap; ++i) {
		assert(0 == nftp_vec_pop(v1, (void **)&e, NFTP_HEAD));
		assert(earr[k ++ % 3] == e);
		assert(cap-i-1 == nftp_vec_len(v1));
	}

	assert(NFTP_ERR_EMPTY == nftp_vec_pop(v1, (void **)&e, NFTP_HEAD));
	assert(NFTP_ERR_EMPTY == nftp_vec_pop(v1, (void **)&e, NFTP_TAIL));

	// Reverse push and pop
	k=-1;
	for (int i=0; i<cap; ++i) {
		k ++;
		assert(0 == nftp_vec_push(v1, (void *) earr[k % 3], NFTP_HEAD));
		assert(i+1 == nftp_vec_len(v1));
	}

	assert(NFTP_ERR_OVERFLOW == nftp_vec_push(v1, (void *) earr[0], NFTP_TAIL));
	assert(NFTP_ERR_OVERFLOW == nftp_vec_push(v1, (void *) earr[0], NFTP_HEAD));

	k=0;
	for (int i=0; i<cap; ++i) {
		assert(0 == nftp_vec_pop(v1, (void **)&e, NFTP_TAIL));
		assert(earr[k ++ % 3] == e);
		assert(cap-i-1 == nftp_vec_len(v1));
	}

	assert(NFTP_ERR_EMPTY == nftp_vec_pop(v1, (void **)&e, NFTP_HEAD));
	assert(NFTP_ERR_EMPTY == nftp_vec_pop(v1, (void **)&e, NFTP_TAIL));

	// Insert and delete test
	assert(0 == nftp_vec_append(v1, (void *)e2));
	assert(1 == nftp_vec_len(v1));

	k=0;
	for (int i=0; i<cap-1; ++i) {
		k++;
		assert(0 == nftp_vec_insert(v1, (void *)earr[k % 3], 1));
		assert(i+2 == nftp_vec_len(v1));
	}
	assert(NFTP_ERR_OVERFLOW == nftp_vec_insert(v1, (void *)earr[0], 1));
	// get and check
	k=cap+1;
	for (int i=0; i<cap; ++i) {
		k--;
		assert(0 == nftp_vec_get(v1, i, (void **)&e));
		assert(e == earr[k % 3]);
	}
	// getidx and delete
	k=cap+1;
	for (int i=0; i<cap; ++i) {
		k--;
		assert(0 == nftp_vec_getidx(v1, earr[k % 3], &idx));
		assert(idx == 0);
		assert(0 == nftp_vec_delete(v1, (void **)&e, idx));
		assert(e == earr[k % 3]);
	}

	assert(0 == nftp_vec_free(v1));

	return 0;
}

