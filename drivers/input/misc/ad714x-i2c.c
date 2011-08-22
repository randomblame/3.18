/*
 * AD714X CapTouch Programmable Controller driver (I2C bus)
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/input.h>	/* BUS_I2C */
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/pm.h>
#include "ad714x.h"

#ifdef CONFIG_PM
static int ad714x_i2c_suspend(struct device *dev)
{
	return ad714x_disable(i2c_get_clientdata(to_i2c_client(dev)));
}

static int ad714x_i2c_resume(struct device *dev)
{
	return ad714x_enable(i2c_get_clientdata(to_i2c_client(dev)));
}
#endif

static SIMPLE_DEV_PM_OPS(ad714x_i2c_pm, ad714x_i2c_suspend, ad714x_i2c_resume);

static int ad714x_i2c_write(struct ad714x_chip *chip,
			    unsigned short reg, unsigned short data)
{
	struct i2c_client *client = to_i2c_client(chip->dev);
	int error;

	chip->xfer_buf[0] = cpu_to_be16(reg);
	chip->xfer_buf[1] = cpu_to_be16(data);

	error = i2c_master_send(client, (u8 *)chip->xfer_buf,
				2 * sizeof(*chip->xfer_buf));
	if (unlikely(error < 0)) {
		dev_err(&client->dev, "I2C write error: %d\n", error);
		return error;
	}

	return 0;
}

static int ad714x_i2c_read(struct ad714x_chip *chip,
			   unsigned short reg, unsigned short *data)
{
	struct i2c_client *client = to_i2c_client(chip->dev);
	int error;

	chip->xfer_buf[0] = cpu_to_be16(reg);

	error = i2c_master_send(client, (u8 *)chip->xfer_buf,
				sizeof(*chip->xfer_buf));
	if (error >= 0)
		error = i2c_master_recv(client, (u8 *)chip->xfer_buf,
					sizeof(*chip->xfer_buf));

	if (unlikely(error < 0)) {
		dev_err(&client->dev, "I2C read error: %d\n", error);
		return error;
	}

	*data = be16_to_cpup(chip->xfer_buf);
	return 0;
}

static int __devinit ad714x_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct ad714x_chip *chip;

	chip = ad714x_probe(&client->dev, BUS_I2C, client->irq,
			    ad714x_i2c_read, ad714x_i2c_write);
	if (IS_ERR(chip))
		return PTR_ERR(chip);

	i2c_set_clientdata(client, chip);

	return 0;
}

static int __devexit ad714x_i2c_remove(struct i2c_client *client)
{
	struct ad714x_chip *chip = i2c_get_clientdata(client);

	ad714x_remove(chip);

	return 0;
}

static const struct i2c_device_id ad714x_id[] = {
	{ "ad7142_captouch", 0 },
	{ "ad7143_captouch", 0 },
	{ "ad7147_captouch", 0 },
	{ "ad7147a_captouch", 0 },
	{ "ad7148_captouch", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ad714x_id);

static struct i2c_driver ad714x_i2c_driver = {
	.driver = {
		.name = "ad714x_captouch",
		.pm   = &ad714x_i2c_pm,
	},
	.probe    = ad714x_i2c_probe,
	.remove   = __devexit_p(ad714x_i2c_remove),
	.id_table = ad714x_id,
};

static __init int ad714x_i2c_init(void)
{
	return i2c_add_driver(&ad714x_i2c_driver);
}
module_init(ad714x_i2c_init);

static __exit void ad714x_i2c_exit(void)
{
	i2c_del_driver(&ad714x_i2c_driver);
}
module_exit(ad714x_i2c_exit);

MODULE_DESCRIPTION("Analog Devices AD714X Capacitance Touch Sensor I2C Bus Driver");
MODULE_AUTHOR("Barry Song <21cnbao@gmail.com>");
MODULE_LICENSE("GPL");
