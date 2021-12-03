// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include "nftp.h"

/* D. J. Bernstein hash function */
uint32_t
nftp_djb_hashn(const uint8_t * cp, size_t n)
{
    uint32_t hash = 5381;
    while (n--)
        hash = 33 * hash ^ (uint8_t) *cp++;
    return hash;
}

/* Fowler/Noll/Vo (FNV) hash function, variant 1a */
uint32_t
nftp_fnv1a_hashn(const uint8_t * cp, size_t n)
{
    uint32_t hash = 0x811c9dc5;
    while (n--) {
        hash ^= (uint8_t) *cp++;
        hash *= 0x01000193;
    }
    return hash;
}

uint8_t
nftp_crc(uint8_t *data, size_t n)
{
	uint8_t crc = 0xff;
	size_t  i, j;
	for (i = 0; i < n; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if ((crc & 0x80) != 0)
				crc = (uint8_t)((crc << 1) ^ 0x31);
			else
				crc <<= 1;
		}
	}
	return crc;
}

