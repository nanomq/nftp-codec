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

#include "nftp.h"
#include "log4nftp.h"

nftp_iter *
nftp_iter_alloc(int schema, void *src)
{
	switch (schema) {
	case NFTP_SCHEMA_VEC:
		return nftp_vec_iter((nftp_vec *)src);
	case NFTP_SCHEMA_IOVS:
		return nftp_iovs_iter((nftp_iovs *)src);
	default:
		nftp_fatal("Unsupported Schema.");
		return NULL;
	}
}

void
nftp_iter_free(nftp_iter *iter)
{
	iter->free(iter);
}

nftp_iter *
nftp_iter_next(nftp_iter *iter)
{
	if (!iter)
		return NULL;
	return iter->next(iter);
}

nftp_iter *
nftp_iter_prev(nftp_iter *iter)
{
	if (!iter)
		return NULL;
	return iter->prev(iter);
}

