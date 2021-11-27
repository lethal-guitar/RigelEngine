# RetroWave Serial Protocol

### Description
This is a versatile protocol which is self synchronized, and doesn't have headers, length information or timing requirements.

The 8-bit byte stream that will be feed into SPI module will be exploded into multiple 7 bits collection. A flag bit that indicates the data type will be appended to each 7 bits collection.

Data boundary and self synchronization are maintained using SPI chip select commands.

### Data structure

| DATA6 | DATA5 | DATA4 |DATA3 |DATA2 |DATA1 |DATA0 | FLAG |
| --- | --- | --- | --- | --- | --- | --- | --- |
| bit 7 |   |   |  |  |  |  | bit 0 |

`DATA<6:0>`
- Carried data
  - When FLAG = 1, the data will be fed into SPI module on receive
  - When FLAG = 0, the data will be interpreted as control commands
    - 0000000 = SPI CS ON (Logic low)
    - 0000001 = SPI CS OFF (Logic high)

`FLAG`
- Data type flag
  - 0 = Control
  - 1 = SPI

### Example
- Host
  - Wants to send bytestream `0xca 0xfe 0xba 0xbe` to device's SPI
  - First byte: `FLAG` = 0, `DATA<6:0>` = 0000000 (SPI CS ON), so it's `0x00`
  - Loop over each **bit** of the bytestream, pad a 1 (`FLAG` = 1) after each 7 bits
    - When there are less than 7 bits left, pad them with anything you like to 7 bits
  - Last byte: `FLAG` = 0, `DATA<6:0>` = 0000001 (SPI CS OFF), so it's `0x02`
  - The bytestream is encoded into `0x00 0xcb 0x7f 0xaf 0x57 0xe1 0x02`
    
- Device
  - Receives the encoded data and loop over each byte
  - When `FLAG` of current byte is 1:
    - Create a 2-byte (16 bits) data buffer, and push back 7 bits from `DATA<6:0>`
    - When the buffer contains >=8 bits of data, send to SPI module
  - When `FLAG` of current byte is 0, do control commands
    - When `DATA<6:0>` = 0000001 (SPI CS OFF), clear the data buffer mentioned above. And synchronization is achieved.
