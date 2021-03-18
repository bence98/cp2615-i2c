// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * i2c support for Silicon Labs' CP2615 Digital Audio Bridge
 *
 * (c) 2021, Bence Csókás <bence98@sch.bme.hu>
 */

#include <linux/i2c.h>
#include <linux/usb.h>
#include "cp2615_iop.h"

#ifndef I2C_AQ_NO_REP_START
/* NO_REP_START adapter quirk not supported by your kernel */
#define I2C_AQ_NO_REP_START 0
#endif //I2C_AQ_NO_REP_START

static int
cp2615_i2c_send(struct usb_interface *usbif, struct cp2615_i2c_transfer *i2c_w)
{
	struct cp2615_iop_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int res = cp2615_init_i2c_msg(msg, i2c_w);

	if (!res)
		res = usb_bulk_msg(usbdev, usb_sndbulkpipe(usbdev, IOP_EP_OUT),
				   msg, ntohs(msg->length), NULL, 0);
	kfree(msg);
	return res;
}

static int
cp2615_i2c_recv(struct usb_interface *usbif, unsigned char tag, void *buf)
{
	struct cp2615_iop_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	struct cp2615_i2c_transfer_result *i2c_r = (struct cp2615_i2c_transfer_result *)&msg->data;
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int res = usb_bulk_msg(usbdev, usb_rcvbulkpipe(usbdev, IOP_EP_IN),
			       msg, sizeof(struct cp2615_iop_msg), NULL, 0);

	if (res < 0)
		return res;

	if (msg->msg != htons(iop_I2cTransferResult) || i2c_r->tag != tag)
		return -EIO;

	res = cp2615_check_status(i2c_r->status);
	if (res < 0)
		return res;

	memcpy(buf, &i2c_r->data, i2c_r->read_len);
	kfree(msg);
	return 0;
}

static int
cp2615_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct usb_interface *usbif = adap->algo_data;
	int i = 0, ret = 0;
	struct i2c_msg *msg;
	struct cp2615_i2c_transfer i2c_w = {0};

	dev_dbg(&usbif->dev, "Doing %d I2C transactions\n", num);

	for (; !ret && i < num; i++) {
		msg = &msgs[i];

		i2c_w.tag = 0xdd;
		i2c_w.i2caddr = i2c_8bit_addr_from_msg(msg);
		if (msg->flags & I2C_M_RD) {
			i2c_w.read_len = msg->len;
			i2c_w.write_len = 0;
		} else {
			i2c_w.read_len = 0;
			i2c_w.write_len = msg->len;
			memcpy(&i2c_w.data, msg->buf, i2c_w.write_len);
		}
		ret = cp2615_i2c_send(usbif, &i2c_w);
		if (ret)
			break;
		ret = cp2615_i2c_recv(usbif, i2c_w.tag, msg->buf);
	}
	if (ret < 0)
		return ret;
	return i;
}

static u32
cp2615_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm cp2615_i2c_algo = {
	.master_xfer	= cp2615_i2c_master_xfer,
	.functionality	= cp2615_i2c_func,
};

/*
 * This chip has some limitations: one is that the USB endpoint
 * can only receive 64 bytes/transfer, that leaves 54 bytes for
 * the I2C transfer. On top of that, EITHER read_len OR write_len
 * may be zero, but not both. If both are non-zero, the adapter
 * issues a write followed by a read. And the chip does not
 * support repeated START between the write and read phases.
 */
struct i2c_adapter_quirks cp2615_i2c_quirks = {
	.max_write_len = MAX_I2C_SIZE,
	.max_read_len = MAX_I2C_SIZE,
	.flags = I2C_AQ_COMB_WRITE_THEN_READ | I2C_AQ_NO_ZERO_LEN | I2C_AQ_NO_REP_START,
	.max_comb_1st_msg_len = MAX_I2C_SIZE,
	.max_comb_2nd_msg_len = MAX_I2C_SIZE
};

static void
cp2615_i2c_remove(struct usb_interface *usbif)
{
	struct i2c_adapter *adap = usb_get_intfdata(usbif);

	usb_set_intfdata(usbif, NULL);
	i2c_del_adapter(adap);
}

static int
cp2615_i2c_probe(struct usb_interface *usbif, const struct usb_device_id *id)
{
	int ret = 0;
	struct i2c_adapter *adap;
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	ret = usb_set_interface(usbdev, IOP_IFN, IOP_ALTSETTING);
	if (ret)
		return ret;

	adap = devm_kzalloc(&usbif->dev, sizeof(struct i2c_adapter), GFP_KERNEL);
	if (!adap)
		return -ENOMEM;

	strncpy(adap->name, usbdev->serial, sizeof(adap->name));
	adap->owner = THIS_MODULE;
	adap->dev.parent = &usbif->dev;
	adap->dev.of_node = usbif->dev.of_node;
	adap->timeout = HZ;
	adap->algo = &cp2615_i2c_algo;
	adap->quirks = &cp2615_i2c_quirks;
	adap->algo_data = usbif;

	ret = i2c_add_adapter(adap);
	if (ret)
		return ret;

	usb_set_intfdata(usbif, adap);
	return ret;
}

static const struct usb_device_id id_table[] = {
	{ USB_DEVICE_INTERFACE_NUMBER(CP2615_VID, CP2615_PID, IOP_IFN) },
	{ }
};

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver cp2615_i2c_driver = {
	.name = "i2c-cp2615",
	.probe = cp2615_i2c_probe,
	.disconnect = cp2615_i2c_remove,
	.id_table = id_table,
};

module_usb_driver(cp2615_i2c_driver);

MODULE_AUTHOR("Bence Csókás <bence98@sch.bme.hu>");
MODULE_DESCRIPTION("CP2615 I2C bus driver");
MODULE_LICENSE("GPL");
