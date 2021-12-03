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

int
main()
{
	char * demo;
	char str[] = "It's a demo.\n";
	char file[] = "./demo.txt";

	assert(strlen(str) == nftp_file_read(file, &demo));
	assert(0 == strcmp(demo, str));

	assert(strlen(str) == nftp_file_write(file, str, strlen(str)));
	free(demo);
}

