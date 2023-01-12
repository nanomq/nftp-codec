// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//

#include <stdio.h>
#include <string.h>

#include "nftp.h"

#ifdef _WIN32

#define F_OK 0

inline int access(const char *pathname, int mode) {
	return _access(pathname, mode);
}
#else
#include <unistd.h>
#include <libgen.h>
#endif

char *
nftp_file_path(char *fpath)
{
	char * dir;
	if ((dir = malloc(strlen(fpath)+4)) == NULL) return NULL;
#ifdef _WIN32
	char buf[3];
	_splitpath_s(fpath,
		buf,  3,            // Don't need drive
		dir+2,sizeof(dir),  // Just the directory
		NULL, 0,            // Don't need filename
		NULL, 0);
	strcpy(dir, buf);
#else
	strcpy(dir, dirname(fpath));
#endif
	return dir;
}

char *
nftp_file_bname(char *fpath)
{
	char * bname;
	if ((bname = malloc(strlen(fpath)+16)) == NULL) return NULL;
#ifdef _WIN32
	char ext[16];
	_splitpath_s(fpath,
		NULL, 0,    // Don't need drive
		NULL, 0,    // Don't need directory
		bname, strlen(fpath) + 15,  // just the filename
		ext  , 15);
	strcpy(bname+strlen(bname), ext, 15);
#else
	strcpy(bname, basename(fpath));
#endif
	return bname;
}

int
nftp_file_exist(char *fpath)
{
	return (access(fpath, F_OK) == 0);
}

int
nftp_file_newname(char *fname, char **newnamep)
{
	char * newname;
	int len = strlen(fname);

	if ((newname = malloc(sizeof(char) * (len+4))) == NULL) {
		return (NFTP_ERR_MEM);
	}

	strcpy(newname, fname);
	strcpy(newname+len, "_00");

	// Retry up to 100 times if filename unreachable
	for (int i = 1; i < 100; ++i) {
		sprintf(newname+len, "_%02d", i);
		if (0 == nftp_file_exist(newname)) {
			*newnamep = newname;
			return (0);
		}
	}

	*newnamep = NULL;
	return (NFTP_ERR_FILENAME);
}

int
nftp_file_rename(char *sfpath, char *dfpath)
{
	return rename(sfpath, dfpath);
}

int
nftp_file_size(char *fpath, size_t *sz)
{
	FILE * fp;
	size_t filesize;

	if ((fp = fopen(fpath, "rb")) == NULL) {
		nftp_fatal("open error [%s]", fpath);
		return (NFTP_ERR_FILE);
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	fclose(fp);

	*sz = filesize;
	return (0);
}

int
nftp_file_blocks(char *fpath, size_t *blocks)
{
	int rv;
	size_t sz;

	if (0 != (rv = nftp_file_size(fpath, &sz))) {
		return rv;
	}

	*blocks = sz/NFTP_BLOCK_SZ + 1;
	return (0);
}

int
nftp_file_readblk(char *fpath, int n, char **strp, size_t *sz)
{
	FILE * fp;
	char * str;
	size_t filesize;
	size_t blksz;

	if ((fp = fopen(fpath, "rb")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);

	if (n > (int)filesize/NFTP_BLOCK_SZ) {
		return (NFTP_ERR_BLOCKS);
	} else if (n == (int)filesize/NFTP_BLOCK_SZ) {
		blksz = filesize - n*NFTP_BLOCK_SZ;
	} else {
		blksz = NFTP_BLOCK_SZ;
	}

	fseek(fp, n*NFTP_BLOCK_SZ, SEEK_SET);

	str = (char *) malloc(blksz + 1);
	memset(str, '\0', blksz + 1);

	if (blksz != fread(str, 1, blksz, fp)) {
		free(str);
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	*strp = str;
	*sz   = blksz;
	return (0);
}

int
nftp_file_read(char *fpath, char **strp, size_t *sz)
{
	FILE * fp;
	char * str;
	char   txt[1000];
	size_t filesize;

	if ((fp = fopen(fpath, "rb")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
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
	*sz   = filesize;
	return (0);
}

int
nftp_file_write(char * fpath, char * str, size_t sz)
{
	FILE * fp;
	size_t filesize;

	if ((fp = fopen(fpath, "wb")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	filesize = fwrite(str, 1, sz, fp);
	if (filesize != sz) {
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return (0);
}

int
nftp_file_append(char * fpath, char * str, size_t sz)
{
	FILE * fp;
	size_t filesize;

	if ((fp = fopen(fpath, "ab")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	filesize = fwrite(str, 1, sz, fp);
	if (filesize != sz) {
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return 0;
}

int
nftp_file_clear(char * fpath)
{
	FILE * fp;

	if ((fp = fopen(fpath, "wb")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	fclose(fp);
	return (0);
}

int
nftp_file_hash(char *fpath, uint32_t *hashval)
{
	FILE *   fp;
	size_t   sz;
	char *   str;
	int      rv;

	if (0 != (rv = nftp_file_size(fpath, &sz)))
		return rv;

	if ((str = malloc(sz)) == NULL)
		return (NFTP_ERR_MEM);

	if ((fp = fopen(fpath, "rb")) == NULL) {
		nftp_fatal("open error");
		return (NFTP_ERR_FILE);
	}

	if (sz != fread(str, 1, sz, fp)) {
		nftp_fatal("read error");
		return (NFTP_ERR_FILE);
	}

	*hashval = NFTP_HASH((const uint8_t *)str, sz);

	fclose(fp);
	free(str);

	return (0);
}

