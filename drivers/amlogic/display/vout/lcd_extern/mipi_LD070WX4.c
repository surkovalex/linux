/*
 * AMLOGIC lcd external driver.
 *
 * Communication protocol:
 * MIPI 
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/jiffies.h> 
#include <linux/i2c.h>
#include <linux/i2c-aml.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <asm/uaccess.h>
#include <mach/pinmux.h>
#include <mach/gpio.h>
#include <linux/amlogic/vout/aml_lcd_extern.h>

static struct lcd_extern_config_t *lcd_ext_config = NULL;

//#define LCD_EXT_DEBUG_INFO
#ifdef LCD_EXT_DEBUG_INFO
#define DBG_PRINT(...)		printk(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

#define LCD_EXTERN_NAME			"lcd_mipi_LD070WX4"

static unsigned char mipi_init_table[] = {
    2,0x01,0x0,
    0xff,0x20,//mdelay flag
    2,0xAE,0x0B,
    2,0xEE,0xEA,
    2,0xEF,0x5F,
    2,0xF2,0x68,
    2,0xEE,0x0,
    2,0xEF,0x0,
    0xff,0xff,//ending flag
};

static int lcd_extern_driver_update(void)
{
    struct aml_lcd_extern_driver_t* lcd_ext;

    lcd_ext = aml_lcd_extern_get_driver();
    if (lcd_ext) {
        lcd_ext->type       = lcd_ext_config->type;
        lcd_ext->name       = lcd_ext_config->name;
        lcd_ext->init_on_cmd_8 = &mipi_init_table[0];
    }
    else {
        printk("[error] %s get lcd_extern_driver failed\n", lcd_ext_config->name);
    }

    return 0;
}

static int aml_LD070WX4_probe(struct platform_device *pdev)
{
    int i = 0;

    if (lcd_extern_driver_check()) {
        return -1;
    }
    if (lcd_ext_config == NULL)
        lcd_ext_config = kzalloc(sizeof(*lcd_ext_config), GFP_KERNEL);
    if (lcd_ext_config == NULL) {
        printk("[error] %s probe: failed to alloc data\n", LCD_EXTERN_NAME);
        return -1;
    }

    pdev->dev.platform_data = lcd_ext_config;

    if (get_lcd_extern_dt_data(pdev->dev.of_node, lcd_ext_config) != 0) {
        printk("[error] %s probe: failed to get dt data\n", LCD_EXTERN_NAME);
        goto lcd_extern_probe_failed;
    }
    lcd_extern_driver_update();

    printk("%s probe ok\n", LCD_EXTERN_NAME);
    return 0;

lcd_extern_probe_failed:
    if (lcd_ext_config)
        kfree(lcd_ext_config);
    return -1;
}

static int aml_LD070WX4_remove(struct platform_device *pdev)
{
    if (pdev->dev.platform_data)
        kfree (pdev->dev.platform_data);
    return 0;
}

#ifdef CONFIG_USE_OF
static const struct of_device_id aml_LD070WX4_dt_match[]={
    {
        .compatible = "amlogic,lcd_mipi_LD070WX4",
    },
    {},
};
#else
#define aml_LD070WX4_dt_match NULL
#endif

static struct platform_driver aml_LD070WX4_driver = {
    .probe  = aml_LD070WX4_probe,
    .remove = aml_LD070WX4_remove,
    .driver = {
        .name  = LCD_EXTERN_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_USE_OF
        .of_match_table = aml_LD070WX4_dt_match,
#endif
    },
};

static int __init aml_LD070WX4_init(void)
{
    int ret;
    DBG_PRINT("%s\n", __FUNCTION__);

    ret = platform_driver_register(&aml_LD070WX4_driver);
    if (ret) {
        printk("[error] %s failed to register lcd extern driver module\n", __FUNCTION__);
        return -ENODEV;
    }

    return ret;
}

static void __exit aml_LD070WX4_exit(void)
{
    platform_driver_unregister(&aml_LD070WX4_driver);
}

//late_initcall(aml_LD070WX4_init);
module_init(aml_LD070WX4_init);
module_exit(aml_LD070WX4_exit);

MODULE_AUTHOR("AMLOGIC");
MODULE_DESCRIPTION("LCD Extern driver for LD070WX4");
MODULE_LICENSE("GPL");
