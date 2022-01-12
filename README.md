# nftp-codec

[![Workflows](https://github.com/nanomq/nftp-codec/actions/workflows/test.yml/badge.svg)](https://github.com/nanomq/nftp-codec/actions)

It's a lightweight ftp parser, and provide a convenient **maker & handler** mechanism to make or handle the package comply to nftp.

## Features

* Support P to P / N to 1 / 1 to N

* Lightweight and no runtime

* Embeddable

* Support asynchronous send / recv

* Support discontinued transmission(TODO)

* Nolock(TODO)

## Quick start

Sender

```
#include "nftp.h"

int sender() {
	int rv;
	char * fname = "demo.txt";
	uint8_t * s; // something to send
	uint8_t * r; // something to recv
	size_t slen; // length of s
	size_t rlen; // length of r

	nftp_proto_init();
	nftp_proto_send_start(fname);
	nftp_proto_maker(fname, NFTP_TYPE_HELLO, 0, &s, &slen);
	test_send(s, slen); // your send function. for example tcp_send()

	test_recv(&r, &rlen); // your recv function. for example tcp_recv();
	rv = nftp_proto_handler(r, rlen, &s, &slen);
	if (rv && s == NULL) {
		nftp_file_blocks(fname, &blocks);
		for (int i=1; i<blocks; ++i) {
			nftp_proto_maker(fname, NFTP_TYPE_FILE, i, &s, &slen);
			test_send(s, slen);
			free(s);
		}
	}

	nftp_proto_fini();
}
```

Recver

```
#include "nftp.h"

int recver() {
	int rv, blocks;
	char * fname = "demo.txt";
	uint8_t * s; // something to send
	uint8_t * r; // something to recv
	size_t slen; // length of s
	size_t rlen; // length of r

	nftp_proto_init();

	while (1) {
		test_recv(&r, &rlen); // your recv function. for example tcp_recv();
		nftp_proto_handler(r, rlen, &s, &slen);
		if (rv == 0) {
			if (s) {
				test_send(s, slen);
				free(s);
			}
		} else {
			printf("Not comply to nftp.\n");
		}
	}

	nftp_proto_fini();
}
```

## How it works

### Process

```
(sync)
sender  --HELLO-->  recver
sender  <---ACK---  recver

(async allowed)
sender  ---FILE-->  recver
sender  ---FILE-->  recver
sender  ---FILE-->  recver
sender  ---END--->  recver
```

### How do discontinued transmission

A good question! TODO :)

## TODO List

* NFTP Version 1.0 standard docs

* Multi-thread

* And so on...

