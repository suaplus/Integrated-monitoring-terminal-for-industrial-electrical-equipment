#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/spi/spi.h>

static struct spi_device *spi_device = NULL;

static int init_adxl345(void)
{
    int ret = 0;
    unsigned char txbuf[2] = {0};

    txbuf[0] = 0x31;
    txbuf[1] = 0x2B;
    ret = spi_write_then_read(spi_device, txbuf, 2, NULL, 0);
    if (ret)
    {
        pr_info("spi_write_then_read failed\n");
        return -1;
    }

    txbuf[0] = 0x2D;
    txbuf[1] = 0x28;
    ret = spi_write_then_read(spi_device, txbuf, 2, NULL, 0);
    if (ret)
    {
        pr_info("spi_write_then_read failed\n");
        return -1;
    }

    txbuf[0] = 0x2E;
    txbuf[1] = 0x00;
    ret = spi_write_then_read(spi_device, txbuf, 2, NULL, 0);
    if (ret)
    {
        pr_info("spi_write_then_read failed\n");
        return -1;
    }

    return 0;
}

static int read_adxl345_xyz(short *pdata, int num)
{
    int ret = 0;
    unsigned char rxbuf[7];
    unsigned char txbuf[7];
    struct spi_message sendmessage;
    struct spi_transfer transfer = {
        .tx_buf = txbuf,
        .rx_buf = rxbuf,
        .len = 7,
        .delay_usecs = 20,
    };

    spi_message_init(&sendmessage);

    txbuf[0] = 0x32 | 0x80 | 0x40;
    spi_message_add_tail(&transfer, &sendmessage);

    ret = spi_sync(spi_device, &sendmessage);
    if (ret)
    {
        pr_info("spi_async failed\n");
        return -1;
    }

    pdata[0] = ((rxbuf[2] << 8) | (rxbuf[1]));
    pdata[1] = ((rxbuf[4] << 8) | (rxbuf[3]));
    pdata[2] = ((rxbuf[6] << 8) | (rxbuf[5]));

    return 0;
}

static ssize_t adxl345_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    int ret = 0;
    short data[3] = {0};
    unsigned long nret = 0;

    ret = read_adxl345_xyz(data, 3);
    if (ret)
    {
        pr_info("read_adxl345_xyz failed\n");
        return -1;
    }

    nret = copy_to_user(puser, data, sizeof(data));
    if (nret)
    {
        pr_info("copy_to_user failed\n");
        return -1;
    }

    return sizeof(data);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
};

static struct miscdevice misc_adxl345 = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "misc_adxl345",
    .fops = &fops,
};

static int adxl345_probe(struct spi_device *spi)
{
    int ret = 0;

    spi_device = spi;
    ret = misc_register(&misc_adxl345);
    if (ret)
    {
        pr_info("misc_register failed\n");
        return -1;
    }

    ret = init_adxl345();
    if (ret)
    {
        pr_info("init_adxl345 failed\n");
        return -1;
    }

    pr_info("probe ok\n");

    return 0;
}

static int adxl345_remove(struct spi_device *spi)
{
    int ret = 0;

    ret = misc_deregister(&misc_adxl345);
    if (ret)
    {
        pr_info("misc_deregister failed\n");
        return -1;
    }

    pr_info("remove ok\n");

    return 0;
}

static struct spi_device_id adxl345_id_table[] = {
    {.name = "puteadxl345"},
    {},
};

static struct of_device_id adxl345_of_match_table[] = {
    {.compatible = "pt,adxl345"},
    {},
};

static struct spi_driver adxl345_drv = {
    .probe = adxl345_probe,
    .remove = adxl345_remove,
    .driver =
        {
            .name = "puteadxl345",
            .owner = THIS_MODULE,
            .of_match_table = adxl345_of_match_table,
        },
    .id_table = adxl345_id_table,
};

module_spi_driver(adxl345_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pute");