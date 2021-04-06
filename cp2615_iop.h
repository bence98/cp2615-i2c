/** CP2615 I/O Protocol implementation
 *  (c) 2021, Bence Csókás <bence98@sch.bme.hu>
 *  Licensed under GPLv2
 *  Source: https://www.silabs.com/documents/public/application-notes/an1139-cp2615-io-protocol.pdf
 */

#ifndef CP2615_IOP_H
#define CP2615_IOP_H

#define CP2615_VID 0x10c4
#define CP2615_PID 0xeac1

#define IOP_EP_IN  0x82
#define IOP_EP_OUT 0x02
#define IOP_IFN 1
#define IOP_ALTSETTING 2

#define MAX_IOP_SIZE 64
#define MAX_IOP_PAYLOAD_SIZE (MAX_IOP_SIZE - 6)
#define MAX_I2C_SIZE (MAX_IOP_PAYLOAD_SIZE - 4)

enum cp2615_iop_msg_type {
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

struct __packed cp2615_iop_msg {
	__be16 preamble, length, msg;
	u8 data[MAX_IOP_PAYLOAD_SIZE];
};

int cp2615_init_iop_msg(struct cp2615_iop_msg *ret, enum cp2615_iop_msg_type msg,
			const void *data, size_t data_len);

#define PART_ID_A01 0x1400
#define PART_ID_A02 0x1500

struct __packed cp2615_iop_accessory_info {
	__be16 part_id, option_id, proto_ver;
};

struct __packed cp2615_i2c_transfer {
	u8 tag, i2caddr, read_len, write_len;
	u8 data[MAX_I2C_SIZE];
};

/*
 * Possible values for struct cp2615_i2c_transfer_result.status
 *
 *  Values extracted from the USBXpress(r) SDK
 */
enum cp2615_i2c_status {
	/* Writing to the internal EEPROM failed, because it is locked */
	CP2615_CFG_LOCKED = -6,
	/* read_len or write_len out of range */
	CP2615_INVALID_PARAM = -4,
	/* I2C slave did not ACK in time */
	CP2615_TIMEOUT,
	/* I2C bus busy */
	CP2615_BUS_BUSY,
	/* I2C bus error (ie. device NAK'd the request) */
	CP2615_BUS_ERROR,
	CP2615_SUCCESS
};

struct __packed cp2615_i2c_transfer_result {
	u8 tag, i2caddr;
	s8 status;
	u8 read_len;
	u8 data[MAX_I2C_SIZE];
};

int cp2615_init_i2c_msg(struct cp2615_iop_msg *ret, const struct cp2615_i2c_transfer *data);

/* Translates status codes to Linux errno's */
int cp2615_check_status(enum cp2615_i2c_status status);

#endif //CP2615_IOP_H
