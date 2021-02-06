#include <libusb-1.0/libusb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "cp2615_iop.h"

#define TOUT_MS 2000

#define PERROR(fn, ...) res=fn(__VA_ARGS__); \
if(res<0){ \
	fprintf(stderr, "Error in %s: %s\n", #fn, libusb_strerror(res));\
	exit(1);\
}

int write_IOP_msg(libusb_device_handle* dev, struct IOP_msg *msg){
	return libusb_bulk_transfer(dev, IOP_EP_OUT, (char*)msg, ntohs(msg->length), NULL, TOUT_MS);
}

int read_IOP_msg(libusb_device_handle* dev, struct IOP_msg *msg){
	return libusb_bulk_transfer(dev, IOP_EP_IN, (char*)msg, sizeof(struct IOP_msg), NULL, TOUT_MS);
}

int main(void){
	libusb_context* ctx;
	libusb_init(&ctx);
	libusb_device_handle* dev=libusb_open_device_with_vid_pid(ctx, CP2615_VID, CP2615_PID);
	libusb_claim_interface(dev, IOP_IFN);
	libusb_set_interface_alt_setting(dev, IOP_IFN, IOP_ALTSETTING);
	
	struct IOP_msg msg;
	int res;
	PERROR(init_IOP_msg, &msg, iop_GetAccessoryInfo, NULL, 0);
	
	PERROR(write_IOP_msg, dev, &msg);
	PERROR(read_IOP_msg, dev, &msg);
	assert(msg.msg==htons(iop_AccessoryInfo));
	struct IOP_AccessoryInfo *info=&msg.data;
	const char* part_str;
	switch(ntohs(info->part_id)){
		case PART_ID_A01:
		part_str="A01";
		break;
		case PART_ID_A02:
		part_str="A02";
		break;
		default:
		part_str="Unknown";
	}
	printf("part %s, option_id: %04X proto_ver: %04X\n", part_str, info->option_id, info->proto_ver);
	
	struct IOP_I2cTransfer i2c_w={
		.tag=0x5A,
//        .i2caddr=0b1100011,
//        .read_len=8,
//        .write_len=1,
//        .data={0x10}
		.i2caddr=0x40,
		.read_len=1,
		.write_len=2,
		.data={0x84, 0xb8}
	};
	
	PERROR(init_IOP_I2c_msg, &msg, &i2c_w);
	PERROR(write_IOP_msg, dev, &msg);
	PERROR(read_IOP_msg, dev, &msg);
	assert(msg.msg==htons(iop_I2cTransferResult));
	struct IOP_I2cTransferResult *i2c_r=&msg.data;
//    printf("I2C status %02X\nSi4713 Status %02X\n\tFW %d.%d.%d\n\tComponent %d.%d\n\tChip rev %d\n",
	printf("I2C status %02X\nSi7050 FW %02X\n",
		i2c_r->status,
		i2c_r->data[0]//,
		//i2c_r->data[1],
		//i2c_r->data[2],
		//i2c_r->data[3]<<8 | i2c_r->data[4],
		//i2c_r->data[5],
		//i2c_r->data[6],
		//i2c_r->data[7]
	);
	
	libusb_release_interface(dev, IOP_IFN);
	libusb_close(dev);
	
	return 0;
}
