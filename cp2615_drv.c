#include <linux/i2c.h>
#include "cp2615_iop.h"

static int
cp2615_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    
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

static int
cp2615_i2c_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adap = platform_get_drvdata(pdev);
    i2c_del_adapter(adap);
	kfree(adap);

	return 0;
}

static int
cp2615_i2c_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct i2c_adapter *adap;

	adap = kzalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
	if (!adap) {
		ret = -ENOMEM;
		goto out;
	}

    memcpy(adap->name, pdev->name, strlen(pdev->name));
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->nr = pdev->id;
    adap->timeout = HZ;
	adap->algo = &iop3xx_i2c_algo;
    
    platform_set_drvdata(pdev, adap);
//	adap->algo_data = adapter_data;

    i2c_add_numbered_adapter(adap);
out:
    return ret;
}

static struct platform_driver cp2615_i2c_driver = {
	.probe		= cp2615_i2c_probe,
	.remove		= cp2615_i2c_remove,
	.driver		= {
		.name	= "CP2615-I2C",
//		.of_match_table = i2c_cp2615_match,
	},
};

module_platform_driver(cp2615_i2c_driver);

MODULE_AUTHOR("Bence Csókás <bence98@sch.bme.hu>");
MODULE_DESCRIPTION("CP2615 I2C bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:CP2615-I2C");
