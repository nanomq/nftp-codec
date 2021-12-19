// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <assert.h>
#include <string.h>

#include "nftp.h"

int
test_iovs()
{
	log("test_iovs");
	nftp_iovs *iovs;
	nftp_iovs *iovs1;
	char *     str  = "bacdef";
	char *     str1 = "bcdefg";
	char *     str2;
	char *     str3 = "g";
	size_t     sz;

	assert(0 == nftp_iovs_alloc(&iovs));
	assert(0 == nftp_iovs_len(iovs));
	assert(NFTP_SIZE == nftp_iovs_cap(iovs));

	assert(0 == nftp_iovs_append(iovs, str, 1)); // b
	assert(1 == nftp_iovs_len(iovs));
	assert(NFTP_SIZE == nftp_iovs_cap(iovs));

	assert(0 == nftp_iovs_push(iovs, str + 1, 1, NFTP_HEAD)); // ab
	assert(2 == nftp_iovs_len(iovs));
	assert(NFTP_SIZE == nftp_iovs_cap(iovs));

	assert(0 == nftp_iovs_push(iovs, str + 2, 4, NFTP_TAIL)); // abcdef
	assert(3 == nftp_iovs_len(iovs));
	assert(NFTP_SIZE == nftp_iovs_cap(iovs));

	assert(0 == nftp_iovs_pop(iovs, (void **)&str2, &sz, NFTP_HEAD)); // bcdef
	assert(2 == nftp_iovs_len(iovs));
	assert('a' == *str2);
	assert(1 == sz);

	assert(0 == nftp_iovs_alloc(&iovs1));
	assert(0 == nftp_iovs_len(iovs1));
	assert(NFTP_SIZE == nftp_iovs_cap(iovs1));

	assert(0 == nftp_iovs_push(iovs1, str3, 1, NFTP_HEAD)); // g
	assert(1 == nftp_iovs_len(iovs1));

	assert(0 == nftp_iovs_cat(iovs, iovs1)); // bcdefg
	assert(3 == nftp_iovs_len(iovs));

	assert(0 == nftp_iovs2stream(iovs, (uint8_t **)&str2, &sz));
	assert(0 == strncmp(str2, str1, strlen(str1)));

	assert(0 == nftp_iovs_free(iovs));
	assert(0 == nftp_iovs_free(iovs1));
	free(str2);

	return (0);
}

