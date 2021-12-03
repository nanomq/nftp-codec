// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <string.h>

#include "nftp.h"

int
nftp_file_read(char *fname, char **strp)
{
	FILE *fp;
	char *str;
	char  txt[1000];
	int   filesize;
	if ((fp = fopen(fname, "r")) == NULL) {
		fatal("open error");
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	str = (char *) malloc(filesize + 1);
	memset(str, '\0', filesize + 1);

	while ((fgets(txt, 1000, fp)) != NULL) {
		strcat(str, txt);
	}

	fclose(fp);
	*strp = str;
	return filesize;
}

int
nftp_file_write(char * fname, char * str, size_t sz)
{
	FILE * fp;
	int    filesize;

	if ((fp = fopen(fname, "w")) == NULL) {
		fatal("open error");
	}

	filesize = fwrite(str, 1, sz, fp);

	fclose(fp);
	return filesize;
}

uint32_t
nftp_file_hash(char *fname)
{
	FILE *   fp;
	size_t   sz = 1000;
	char     txt[sz];
	int      pos;
	uint32_t res = 5381;

	if ((fp = fopen(fname, "r")) == NULL) {
		fatal("read error");
	}

	while ((fgets(txt, sz, fp)) != NULL) {
		for (pos = 0; pos < sz; ++pos) {
			res = 33 * res ^ (uint8_t) txt[pos];
		}
	}

	fclose(fp);
	return res;
}

