# nftp-codec

[![Workflows](https://github.com/nanomq/nftp-codec/actions/workflows/test.yml/badge.svg)](https://github.com/nanomq/nftp-codec/actions)

It's a lightweight NFTP(Nano FTP) ver1.1 protocol parser.

## NFTP ver2.0 was released at [Here](https://github.com/nanomq/nftp-codec/blob/main/docs/doc-ver2.0.md) !!! Submit your issues to make NFTP better.

## Features

* Support P to P / N to 1 / 1 to N

* Lightweight and no runtime

* Support asynchronous send / recv

* Support discontinued transmission

* NFTP Version 1.1 standard doc is at [Here](https://github.com/nanomq/nftp-codec/blob/main/docs/doc-ver1.1.md)

## Quick start

Here are some demo to use nftp-codec.

NFTP over TCP [nftp-go](https://github.com/wanghaEMQ/nftp-go)

NFTP over MQTT [nftp-mqtt](https://github.com/nanomq/nanosdk)

[...] More demos are in developing.

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

### Something you should know

|  Property   | iter | vector | iovs | codec | file | hash | proto |
| :---------: | :--: | :----: | :--: | :---: | :--: | :--: | :---: |
| Thread-safe |  X   |   O    |  O   |   O   |  X   |  O   |   X   |

## TODO List

* More easy to use
* Support NFTP ver2.0
* And so on...

