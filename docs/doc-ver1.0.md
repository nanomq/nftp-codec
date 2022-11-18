# Doc of NFTP

## Protocol Summary

<table style="text-align:center">
  <tr>
    <td align="center"><strong>Byte</strong></td>
    <td align="center"><strong>0</strong></td>
    <td align="center"><strong>1</strong></td>
    <td align="center"><strong>2</strong></td>
    <td align="center"><strong>3</strong></td>
    <td align="center"><strong>4</strong></td>
    <td align="center"><strong>5</strong></td>
  </tr>
  <tr>
    <td>byte 0-5</td>
    <td colspan="1" align="center">NFTP Type (1B)</td>
    <td colspan="4" align="center">Length (4B)</td>
    <td colspan="1" align="center">ID (1B)</td>
  </tr>
  <tr>
    <td>byte 6-a</td>
    <td colspan="1" align="center">Blocks (Option, 1B)</td>
    <td colspan="5" align="center">FileName (Option, Uft8-String)</td>
  </tr>
  <tr>
    <td>byte a-b</td>
    <td colspan="4" align="center">FileFlag (Option, 4B)</td>
    <td colspan="2" align="center">Content (Option, Binary Stream)</td>
  </tr>
  <tr>
    <td>byte b-n</td>
    <td colspan="5" align="center">Content (Option, Binary Stream)</td>
    <td colspan="1" align="center">CRC (Option, 1B)</td>
  </tr>
</table>

## Type of Packet

| Type   | Value (Hex) | Description                                    | Direction        |
| ------ | ----------- | ---------------------------------------------- | ---------------- |
| HELLO  | 0x01        | Always the first packet of Transferring.       | Sender -> Recver |
| ACK    | 0x02        | An acknowledge of HELLO packet.                | Recver -> Sender |
| FILE   | 0x03        | The FILE packet contains the contents of file. | Sender -> Recver |
| END    | 0x04        | The last packet contain contents of file.      | Sender -> Recver |
| GIVEME | 0x05        | Ask a packet with certain ID.                  | Recver -> Sender |

## Details of Packet

### HELLO Packet

+ The HELLO packet is the first packet of transferring for one file.

+ Fixed Header

Its type field is (0x01).

Its length field is the total length of this packet.

Its ID field is always (0x00).

+ More fields

Followed with Blocks field, FileName field and Content field.

Its Blocks field is the counter of blocks for the file. We send a FILE packet with one block at a time.

Its FileName field is the name of file we want to transfer. The FileName field is a uft8-string (Same as utf8-string in MQTT).

Its Content field contains 4 Bytes of hashcode of File.

+ Actions

Sender **MUST** drop it If received a HELLO packet. After decoding the HELLO packet, something followed should be done.

Check the ID, the ID **MUST** be (0x00)

Binding the callback user set before for the filename if necessary.

Create a part file for storing. It should be deleted after finishing receive.

Better to return a ACK packet for responsing.

### ACK Packet

+ The ACK packet is a acknowledge of HELLO packet.
+ Fixed header.

Its type field is (0x02).

 Its length field is the total length of this packet.

Its ID field always ZERO (0x00).

+ More fields

Followed with Fileflag.

Its FileFlag field is the DJBHash value of FileName.

+ Action

Recver **MUST** drop it If received a ACK packet. After decoding the ACK packet, something followed should be done.

Check the ID, the ID **MUST** be (0x00).

### FILE Packet

+ The FILE packet contains the contents of file. And a block contained in one Pakcet. The size of block is define in nftp.h ((256 * 1024) Default).
+ Fixed header.

Its type field is (0x03).

 Its length field is the total length of this packet.

Its ID field start from 1 (0x01).

+ More fields

Followed with Fileflag field, content field and CRC field.

Its FileFlag field which get from ACK packet.

Its Content field is the ID th block of file. For example. If ID is 3, then the content is the (256\*1024) bytes from 2\*(256\*1024).

Its CRC field is the Hashcode value of Content field.

+ Actions

Sender **MUST** drop it If received a FILE packet. After decoding the FILE packet, something followed should be done.

Check the ID, the ID **MUST NOT** be (0x00)

Packets from multiple files can be transferred at same time.

User can adjust the maximal number of files transferring. (Avoid OOM)

### END Packet

+ The END Packet is the last packet contain contents of file. It means the ending of transferring for a file.
+ Fixed Header

All fields are same to the fields in FILE Packet beside type field. The END Packet field is (0x04).

+ More fields

All fields are same to the fields in FILE Packet.

+ Actions

All actions are same to the actions in FILE Packet.

### GIVEME Packet

+ The GIVEME packet is use to ask a packet with certain ID from Recver. 
+ Fixed header.

Its type field is (0x05).

Its length field is the total length of this packet.

Its ID field is the ID of Packet we wanted.

+ More fields

Followed with FileName field.

Its FileName field contains the FileName which packet comes from.

+ Actions

Recver **MUST** drop it If received a GIVEME packet. After decoding the GIVEME packet, something followed should be done.

Check the ID, the ID **MUST NOT** be (0x00). Or we should stop transferring.

Return a FILE/END Packet for caller.

