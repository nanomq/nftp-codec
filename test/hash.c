// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <assert.h>
#include <stdio.h>

#include "nftp.h"

int
test_hash()
{
	assert(nftp_djb_hashn((uint8_t *) "asdfghjkl", 9) ==
	    nftp_djb_hashn((uint8_t *) "asdfghjkl", 9));
	assert(nftp_fnv1a_hashn((uint8_t *) "asdfghjkl", 9) ==
	    nftp_fnv1a_hashn((uint8_t *) "asdfghjkl", 9));
	assert(nftp_crc((uint8_t *) "asdfghjkl", 9) ==
	    nftp_crc((uint8_t *) "asdfghjkl", 9));

	uint32_t a = nftp_djb_hashn((uint8_t *) "ab.c", 4);
	uint8_t a1 = (uint8_t) (0xff & (a >> 24u));
	uint8_t a2 = (uint8_t) (0xff & (a >> 16u));
	uint8_t a3 = (uint8_t) (0xff & (a >> 8u));
	uint8_t a4 = (uint8_t) (0xff & a);
	// log("%x%x%x%x", a1, a2, a3, a4);

	return (0);
}

