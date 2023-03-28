# Doc of NFTP Version2.0


## Type of Packet

| Type   | Value (Hex) | Description                                    | Direction        |
| ------ | ----------- | ---------------------------------------------- | ---------------- |
| HELLO  | 0x01        | Always the first packet of Transferring.       | Sender -> Recver |
| ACK    | 0x02        | An acknowledge of HELLO packet.                | Recver -> Sender |
| FILE   | 0x03        | The FILE packet contains the contents of file. | Sender -> Recver |
| END    | 0x04        | The last packet.                               | Sender -> Recver |
| GIVEME | 0x05        | Ask packets with certain IDs.                  | Recver -> Sender |

## Process of transmission

```
(sync)
sender  --HELLO-->  recver
sender  <--ACK----  recver

(async allowed)
sender  ---FILE-->  recver
sender  ---FILE-->  recver
sender  <-GIVEME--  recver (when error in checksum of FILE)
sender  ---FILE-->  recver

(sync)
sender  ---END--->  recver
sender  <-GIVEME--  recver

(async, send packets if some packets lost, or process is done)
sender  ---FILE-->  recver
sender  ---FILE-->  recver

(sync)
sender  ---END--->  recver
sender  <-GIVEME--  recver
```

### Note

* If sender did NOT receive an ACK after sending a HELLO packet. Sender should wait at least **5s** before resending HELLO packet.
* If sender did NOT receive an GIVEME after sending a END packet. Sender should wait at least **5s** before resending END packet.
* Sender should **NOT** send FILE packets before received the ACK.
* GIVEME packet from the recver asks more than or equal to ZERO FILE packets. (>=0)
* The fileId should **NOT** be expired before the sender received a GIVEME packet which asks ZERO FILE packet.(==0)
* CRC32C is the default checksum hash.

## Details of Packets

### HELLO Packet

The HELLO packet is always the first packet of transmission.

| Name            | Length(Byte)    | Description                              |
| --------------- | --------------- | ---------------------------------------- |
| Type            | 1               | HELLO(0x01)                              |
| Length          | 4               | The size of this packet.                 |
| FileId          | 4               | Given by user. Each one presents a file. |
| Blocks          | 2               | File Blocks number. (Up to 65535)        |
| FileName length | 2               | The length of file name.                 |
| FileName        | FileName length | File name.                               |
| File CRC        | 4               | Checksum of content of file.             |

+ Actions

Sender must drop it If received a HELLO packet.

Recver should create a part file for saving. Which should be deleted after finishing transmission.

Recver must reply an ACK packet with same FileId as response.

### ACK Packet

The ACK packet is an acknowledge of HELLO packet.

| Name   | Length(Byte) | Description                                 |
| ------ | ------------ | ------------------------------------------- |
| Type   | 1            | ACK(0x02)                                   |
| Length | 4            | The size of this packet.                    |
| FileId | 4            | FileId. As same as the one in HELLO packet. |

+ Action

Recver must drop it If received an ACK packet.

### FILE Packet

The FILE packet contains the contents of file. And each block was contained in one Packet. The size of block is define in nftp.h ((32 * 1024) by default).

| Name      | Length(Byte)       | Description                                  |
| --------- | ------------------ | -------------------------------------------- |
| Type      | 1                  | FILE(0x03)                                   |
| Length    | 4                  | The size of this packet.                     |
| FileId    | 4                  | File Id. As same as the one in HELLO packet. |
| Block Seq | 2                  | Block sequence number. Start from 0.         |
| Content   | Length-(1+4+4+2+4) | The content of block.                        |
| Block CRC | 4                  | Checksum of content of block.                |

+ Actions

Sender must drop it If received a FILE packet.

### END Packet

The END packet means the sender has sent all requested packets(for a HELLO or GIVEME).

| Name   | Length(Byte) | Description                                 |
| ------ | ------------ | ------------------------------------------- |
| Type   | 1            | END(0x04)                                   |
| Length | 4            | The size of this packet.                    |
| FileId | 4            | FileId. As same as the one in HELLO packet. |

### GIVEME Packet

The GIVEME packet is use to ask more than or equal to ZERO (>=0) FILE packets with certain IDs from the recver.

| Name       | Length(Byte) | Description                                                  |
| ---------- | ------------ | ------------------------------------------------------------ |
| Type       | 1            | GIVEME(0x05)                                                 |
| Length     | 4            | The size of this packet.                                     |
| FileId     | 4            | File Id. As same as the one in HELLO packet.                 |
| Block Seqs | Blocks/8     | A bitmap of the required packets. Each bit represent a block. |

Recver must drop it If received a GIVEME packet.

