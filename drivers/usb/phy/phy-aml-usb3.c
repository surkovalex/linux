/*
 * phy-aml-usb3.c
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include "phy-aml-usb.h"

static int amlogic_usb3_suspend(struct usb_phy *x, int suspend)
{
#if 0
	struct omap_usb *phy = phy_to_omapusb(x);
	int	val;
	int timeout = PLL_IDLE_TIME;

	if (suspend && !phy->is_suspended) {
		val = omap_usb_readl(phy->pll_ctrl_base, PLL_CONFIGURATION2);
		val |= PLL_IDLE;
		omap_usb_writel(phy->pll_ctrl_base, PLL_CONFIGURATION2, val);

		do {
			val = omap_usb_readl(phy->pll_ctrl_base, PLL_STATUS);
			if (val & PLL_TICOPWDN)
				break;
			udelay(1);
		} while (--timeout);

		omap_control_usb3_phy_power(phy->control_dev, 0);

		phy->is_suspended	= 1;
	} else if (!suspend && phy->is_suspended) {
		phy->is_suspended	= 0;

		val = omap_usb_readl(phy->pll_ctrl_base, PLL_CONFIGURATION2);
		val &= ~PLL_IDLE;
		omap_usb_writel(phy->pll_ctrl_base, PLL_CONFIGURATION2, val);

		do {
			val = omap_usb_readl(phy->pll_ctrl_base, PLL_STATUS);
			if (!(val & PLL_TICOPWDN))
				break;
			udelay(1);
		} while (--timeout);
	}
#endif
	return 0;
}

static int amlogic_usb3_init(struct usb_phy *x)
{
	struct amlogic_usb *phy = phy_to_amlusb(x);

	usb_aml_regs_t *usb_aml_regs;
	usb_r0_t r0 = {.d32 = 0};
	int i;

	amlogic_usbphy_reset();
	
	for(i=0;i<phy->portnum;i++)
	{
		usb_aml_regs = (usb_aml_regs_t *)((unsigned int)phy->regs+i*PHY_REGISTER_SIZE);

		usb_aml_regs->usb_r3 = (1<<13) | (0x68<<24);
		r0.d32 = usb_aml_regs->usb_r0;
		r0.b.p30_phy_reset = 1;
		usb_aml_regs->usb_r0 = r0.d32;
    		udelay(2);
		r0.b.p30_phy_reset = 0;
		usb_aml_regs->usb_r0 = r0.d32;
	}
	return 0;
}

static int amlogic_usb3_probe(struct platform_device *pdev)
{
	struct amlogic_usb			*phy;
	struct device *dev = &pdev->dev;
	struct resource *phy_mem;
	void __iomem	*phy_base;
	int portnum=0;
	const void *prop;
	
	prop = of_get_property(dev->of_node, "portnum", NULL);
	if(prop)
		portnum = of_read_ulong(prop,1);

	if(!portnum)
	{
		dev_err(&pdev->dev, "This phy has no usb port\n");
		return -ENOMEM;
	}
	phy_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	phy_base = devm_ioremap_resource(dev, phy_mem);
	if (IS_ERR(phy_base))
		return PTR_ERR(phy_base);
	phy = devm_kzalloc(&pdev->dev, sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		dev_err(&pdev->dev, "unable to allocate memory for USB3 PHY\n");
		return -ENOMEM;
	}

	printk(KERN_INFO"USB3 phy probe:phy_mem:0x%8x, iomap phy_base:0x%8x	\n",
	               (unsigned int)phy_mem->start,(unsigned int)phy_base);
	
	phy->dev		= dev;
	phy->regs		= phy_base;
	phy->portnum      = portnum;
	phy->phy.dev		= phy->dev;
	phy->phy.label		= "amlogic-usbphy2";
	phy->phy.init		= amlogic_usb3_init;
	phy->phy.set_suspend	= amlogic_usb3_suspend;
	phy->phy.type		= USB_PHY_TYPE_USB3;

	usb_add_phy_dev(&phy->phy);

	platform_set_drvdata(pdev, phy);

	pm_runtime_enable(phy->dev);

	return 0;
}

static int amlogic_usb3_remove(struct platform_device *pdev)
{
#if 0
	struct omap_usb *phy = platform_get_drvdata(pdev);

	clk_unprepare(phy->wkupclk);
	clk_unprepare(phy->optclk);
	usb_remove_phy(&phy->phy);
	if (!pm_runtime_suspended(&pdev->dev))
		pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

#ifdef CONFIG_PM_RUNTIME

static int amlogic_usb3_runtime_suspend(struct device *dev)
{
#if 0
	struct platform_device	*pdev = to_platform_device(dev);
	struct omap_usb	*phy = platform_get_drvdata(pdev);

	clk_disable(phy->wkupclk);
	clk_disable(phy->optclk);
#endif
	return 0;
}

static int amlogic_usb3_runtime_resume(struct device *dev)
{
	u32 ret = 0;
#if 0	
	struct platform_device	*pdev = to_platform_device(dev);
	struct omap_usb	*phy = platform_get_drvdata(pdev);

	ret = clk_enable(phy->optclk);
	if (ret) {
		dev_err(phy->dev, "Failed to enable optclk %d\n", ret);
		goto err1;
	}

	ret = clk_enable(phy->wkupclk);
	if (ret) {
		dev_err(phy->dev, "Failed to enable wkupclk %d\n", ret);
		goto err2;
	}

	return 0;

err2:
	clk_disable(phy->optclk);

err1:
#endif
	return ret;
}

static const struct dev_pm_ops amlogic_usb3_pm_ops = {
	SET_RUNTIME_PM_OPS(amlogic_usb3_runtime_suspend, amlogic_usb3_runtime_resume,
		NULL)
};

#define DEV_PM_OPS     (&amlogic_usb3_pm_ops)
#else
#define DEV_PM_OPS     NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id amlogic_usb3_id_table[] = {
	{ .compatible = "amlogic,amlogic-usb3" },
	{}
};
MODULE_DEVICE_TABLE(of, amlogic_usb3_id_table);
#endif

static struct platform_driver amlogic_usb3_driver = {
	.probe		= amlogic_usb3_probe,
	.remove		= amlogic_usb3_remove,
	.driver		= {
		.name	= "amlogic-usb3",
		.owner	= THIS_MODULE,
		.pm	= DEV_PM_OPS,
		.of_match_table = of_match_ptr(amlogic_usb3_id_table),
	},
};

module_platform_driver(amlogic_usb3_driver);

MODULE_ALIAS("platform: amlogic_usb3");
MODULE_AUTHOR("Amlogic Inc.");
MODULE_DESCRIPTION("amlogic USB3 phy driver");
MODULE_LICENSE("GPL v2");
