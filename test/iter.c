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
test_iter()
{
	nftp_log("test_iter");
	int n1, n2;
	nftp_vec *v;
	nftp_iovs *iovs;
	nftp_iter *iter;
	struct iovec * iov;

	assert(0 == nftp_vec_alloc(&v));
	assert(0 == nftp_vec_append(v, &n1));
	assert(0 == nftp_vec_append(v, &n2));

	assert(NULL != (iter = nftp_iter_alloc(NFTP_SCHEMA_VEC, v)));
	assert(NFTP_HEAD == iter->key);
	assert(NULL == iter->val);

	assert(NULL != nftp_iter_next(iter));
	assert(0 == iter->key);
	assert(&n1 == iter->val);

	assert(NULL != nftp_iter_next(iter));
	assert(1 == iter->key);
	assert(&n2 == iter->val);

	assert(NULL != nftp_iter_next(iter));
	assert(NFTP_TAIL == iter->key);
	assert(NULL == iter->val);

	nftp_vec_free(v);

	assert(0 == nftp_iovs_alloc(&iovs));
	assert(0 == nftp_iovs_append(iovs, &n1, 1));
	assert(0 == nftp_iovs_append(iovs, &n2, 1));

	assert(NULL != (iter = nftp_iter_alloc(NFTP_SCHEMA_IOVS, iovs)));
	assert(NFTP_HEAD == iter->key);
	assert(NULL == iter->val);

	assert(NULL != nftp_iter_next(iter));
	assert(0 == iter->key);
	iov = (struct iovec *) iter->val;
	assert(&n1 == iov->iov_base);
	assert(1 == iov->iov_len);

	assert(NULL != nftp_iter_next(iter));
	assert(1 == iter->key);
	iov = (struct iovec *) iter->val;
	assert(&n2 == iov->iov_base);
	assert(1 == iov->iov_len);

	assert(NULL != nftp_iter_next(iter));
	assert(NFTP_TAIL == iter->key);
	assert(NULL == iter->val);

	assert(0 == nftp_iovs_free(iovs));

	return (0);
}
