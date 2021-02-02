/** CP2615 I/O Protocol implementation
 *  (c) 2021, Bence Csókás <bence98@sch.bme.hu>
 *  Licensed under GPLv2
 *  Source: https://www.silabs.com/documents/public/application-notes/an1139-cp2615-io-protocol.pdf
 */

#ifndef CP2615_IOP_H
#define CP2615_IOP_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#define MAX_IOP_SIZE 64
#define MAX_IOP_PAYLOAD_SIZE MAX_IOP_SIZE-6
#define MAX_I2C_SIZE MAX_IOP_PAYLOAD_SIZE-4

enum IOP_msg_type {
	iop_GetAccessoryInfo = 0xD100,
	iop_AccessoryInfo = 0xA100,
	iop_GetPortConfiguration = 0xD203,
	iop_PortConfiguration = 0xA203,
	// ...
	iop_DoI2cTransfer = 0xD400,
	iop_I2cTransferResult = 0xA400,
	iop_GetSerialState = 0xD501,
	iop_SerialState = 0xA501
};

struct IOP_msg {
	uint16_t preamble, length, msg;
	char data[MAX_IOP_PAYLOAD_SIZE];
};

int init_IOP_msg(struct IOP_msg *ret, enum IOP_msg_type msg, const void *data, size_t data_len);

#define PART_ID_A01 0x1400
#define PART_ID_A02 0x1500

struct IOP_AccessoryInfo {
	uint16_t part_id, option_id, proto_ver;
};

struct IOP_I2cTransfer {
	unsigned char tag, i2caddr, read_len, write_len;
	char data[MAX_I2C_SIZE];
};

struct IOP_I2cTransferResult {
    unsigned char tag, i2caddr, status, read_len;
	char data[MAX_I2C_SIZE];
};

#endif //CP2615_IOP_H
