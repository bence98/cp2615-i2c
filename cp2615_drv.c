#include <linux/i2c.h>
#include <linux/usb.h>
#include "cp2615_iop.h"

static int
cp2615_i2c_send(struct usb_interface *usbif, struct IOP_I2cTransfer *i2c_w)
{
	struct IOP_msg msg;
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int res = init_IOP_I2c_msg(&msg, i2c_w);
	if (res < 0)
		return res;

	// TODO: send via USB
	return usb_bulk_msg(usbdev, usb_sndbulkpipe(usbdev, IOP_EP_OUT), &msg, ntohs(msg.length), NULL, 0);
}

static int
cp2615_i2c_recv(struct usb_interface *usbif, unsigned char tag, void *buf)
{
	struct IOP_msg msg;
	struct IOP_I2cTransferResult *i2c_r = (struct IOP_I2cTransferResult*) &msg.data;
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int res = usb_bulk_msg(usbdev, usb_rcvbulkpipe(usbdev, IOP_EP_IN), &msg, sizeof(struct IOP_msg), NULL, 0);
	if (res < 0)
		return res;

	if (msg.msg != htons(iop_I2cTransferResult) || i2c_r->tag != tag || !i2c_r->status)
		return -1; // TODO

	memcpy(buf, &i2c_r->data, i2c_r->read_len);
	return 0;
}

static int
cp2615_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct usb_interface *usbif = adap->algo_data;
	int i = 0, ret = 0;
	struct i2c_msg *msg;
	struct IOP_I2cTransfer i2c_w = {0};

	for(; !ret && i < num; i++) {
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
		if (ret < 0)
			break;
		ret = cp2615_i2c_recv(usbif, i2c_w.tag, msg->buf);
	}
	return ret;
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

struct i2c_adapter_quirks cp2615_i2c_quirks = {
    .max_write_len = MAX_I2C_SIZE,
    .max_read_len = MAX_I2C_SIZE,
};

static void
cp2615_i2c_remove(struct usb_interface *usbif)
{
	struct i2c_adapter *adap = usb_get_intfdata(usbif);

	usb_set_intfdata(usbif, NULL);
	i2c_del_adapter(adap);
	kfree(adap);
}

static int
cp2615_i2c_probe(struct usb_interface *usbif, const struct usb_device_id *id)
{
	int ret = 0;
	struct i2c_adapter *adap;
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	adap = kzalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
	if (!adap) {
		ret = -ENOMEM;
		goto out;
	}

	strncpy(adap->name, usbdev->serial, sizeof(adap->name));
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	adap->dev.parent = &usbif->dev;
	adap->dev.of_node = usbif->dev.of_node;
	adap->timeout = HZ;
	adap->algo = &cp2615_i2c_algo;
    adap->quirks = &cp2615_i2c_quirks;

	usb_set_intfdata(usbif, adap);
	adap->algo_data = usbif;

	ret = i2c_add_adapter(adap);
	if (!ret)
		goto out;
	ret = usb_set_interface(usbdev, IOP_IFN, IOP_ALTSETTING);
out:
	return ret;
}

static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(CP2615_VID, CP2615_PID) },
	{ }
};

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver cp2615_i2c_driver = {
	.name = "cp2615-i2c",
	.probe = cp2615_i2c_probe,
	.disconnect = cp2615_i2c_remove,
	.id_table = id_table,
//	.dev_groups = cp2615_groups,
};

module_usb_driver(cp2615_i2c_driver);

MODULE_AUTHOR("Bence Csókás <bence98@sch.bme.hu>");
MODULE_DESCRIPTION("CP2615 I2C bus driver");
MODULE_LICENSE("GPL");
