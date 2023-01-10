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
#include <stdlib.h>

#include "nftp.h"
#include "test.h"

int
test_file()
{
	nftp_log("test_file");
	char * demo;
	char str[] = "It's a demo.\n";
	char str2[] = "It's a demo.\nIt's a demo.\n";
	char file[] = "demo.txt";
	size_t sz = 0;
	uint32_t hashval;
	char *newfname;

	if (0 == nftp_file_exist(file)) {
		assert(0 == nftp_file_write(file, "", 0));
	}
	assert(0 == nftp_file_clear(file));

	assert(0 == nftp_file_newname(file, &newfname));
	assert(0 == strcmp("demo.txt_01", newfname));
	free(newfname);

	assert(0 == nftp_file_read(file, &demo, &sz));
	assert(0 == strcmp(demo, ""));
	assert(0 == sz);
	free(demo);

	assert(0 == nftp_file_write(file, str, strlen(str)));
	assert(0 == nftp_file_read(file, &demo, &sz));
	assert(0 == strcmp(demo, str));
	assert(sz == strlen(str));
	free(demo);

	assert(0 == nftp_file_readblk(file, 0, &demo, &sz));
	assert(0 == strcmp(demo, str));
	assert(sz == strlen(str));
	free(demo);

	assert(NFTP_ERR_BLOCKS == nftp_file_readblk(file, 1, &demo, &sz));

	assert(0 == nftp_file_append(file, str, strlen(str)));
	assert(0 == nftp_file_read(file, &demo, &sz));
	assert(0 == strcmp(demo, str2));
	assert(sz == strlen(str2));

	assert(0 == nftp_file_hash(file, &hashval));
	assert(NFTP_HASH((uint8_t *)demo, strlen(demo)) == hashval);

	free(demo);

	return (0);
}

