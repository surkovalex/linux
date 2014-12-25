/*
 * Amlogic LCD Driver
 *
 * Author:
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/amlogic/vout/lcd.h>
#include <linux/amlogic/vout/vinfo.h>
#include <linux/amlogic/vout/vout_notify.h>
#include <linux/amlogic/vout/lcd_aml.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <mach/am_regs.h>
#include <mach/mlvds_regs.h>
#include <mach/clock.h>
#include <asm/fiq.h>
#include <linux/delay.h>
#include <plat/regops.h>
#include <mach/am_regs.h>
#include <mach/hw_enc_clk_config.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/gpio.h>
#include <linux/amlogic/panel.h>
#include <linux/of.h>

static struct class *aml_lcd_clsp;

#define CLOCK_ENABLE_DELAY			500
#define CLOCK_DISABLE_DELAY			50
#define PWM_ENABLE_DELAY			20
#define PWM_DISABLE_DELAY			20
#define PANEL_POWER_ON_DELAY		50
#define PANEL_POWER_OFF_DELAY		0
#define BACKLIGHT_POWER_ON_DELAY	0
#define BACKLIGHT_POWER_OFF_DELAY	200

#define H_ACTIVE			1920
#define V_ACTIVE			1080
#define H_PERIOD			2200
#define V_PERIOD			1125
#define VIDEO_ON_PIXEL	148
#define VIDEO_ON_LINE	41

typedef struct {
	Lcd_Config_t conf;
	vinfo_t lcd_info;
} lcd_dev_t;
static lcd_dev_t *pDev = NULL;

static int cur_lvds_index = 0;
MODULE_PARM_DESC(cur_lvds_index, "\n cur_lvds_index \n");
module_param(cur_lvds_index, int, 0664);

static int pn_swap = 0;
MODULE_PARM_DESC(pn_swap, "\n pn_swap \n");
module_param(pn_swap, int, 0664);

static int dual_port = 1;
MODULE_PARM_DESC(dual_port, "\n dual_port \n");
module_param(dual_port, int, 0664);

static int bit_num_flag = 1;
static int bit_num = 1;
MODULE_PARM_DESC(bit_num, "\n bit_num \n");
module_param(bit_num, int, 0664);

static int lvds_repack_flag = 1;
static int lvds_repack = 1;
MODULE_PARM_DESC(lvds_repack, "\n lvds_repack \n");
module_param(lvds_repack, int, 0664);

static int port_reverse_flag = 1;
static int port_reverse = 1;
MODULE_PARM_DESC(port_reverse, "\n port_reverse \n");
module_param(port_reverse, int, 0664);

int flaga = 0;
MODULE_PARM_DESC(flaga, "\n flaga \n");
module_param(flaga, int, 0664);
EXPORT_SYMBOL(flaga);

int flagb = 0;
MODULE_PARM_DESC(flagb, "\n flagb \n");
module_param(flagb, int, 0664);
EXPORT_SYMBOL(flagb);

int flagc = 0;
MODULE_PARM_DESC(flagc, "\n flagc \n");
module_param(flagc, int, 0664);
EXPORT_SYMBOL(flagc);

int flagd = 0;
MODULE_PARM_DESC(flagd, "\n flagd \n");
module_param(flagd, int, 0664);
EXPORT_SYMBOL(flagd);

void vpp_set_matrix_ycbcr2rgb (int vd1_or_vd2_or_post, int mode)
{
	if (vd1_or_vd2_or_post == 0){ //vd1
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 5, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 8, 2);
	}else if (vd1_or_vd2_or_post == 1){ //vd2
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 4, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 2, 8, 2);
	}else{
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 0, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 0, 8, 2);
		if (mode == 0){
			aml_set_reg32_bits(P_VPP_MATRIX_CTRL, 1, 1, 2);
		}else if (mode == 1){
			aml_set_reg32_bits(P_VPP_MATRIX_CTRL, 0, 1, 2);
		}
	}

	if (mode == 0){ //ycbcr not full range, 601 conversion
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x0064C8FF);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x006400C8);

		//1.164     0       1.596
		//1.164   -0.392    -0.813
		//1.164   2.017     0
		aml_write_reg32(P_VPP_MATRIX_COEF00_01, 0x04000000);
		aml_write_reg32(P_VPP_MATRIX_COEF02_10, 0x059C0400);
		aml_write_reg32(P_VPP_MATRIX_COEF11_12, 0x1EA01D24);
		aml_write_reg32(P_VPP_MATRIX_COEF20_21, 0x04000718);
		aml_write_reg32(P_VPP_MATRIX_COEF22, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_OFFSET0_1, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_OFFSET2, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x00000E00);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x00000E00);
	}else if (mode == 1) {//ycbcr full range, 601 conversion
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x0000600);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x0600);

		//1     0       1.402
		//1   -0.34414  -0.71414
		//1   1.772     0
		aml_write_reg32(P_VPP_MATRIX_COEF00_01, (0x400 << 16) |0);
		aml_write_reg32(P_VPP_MATRIX_COEF02_10, (0x59c << 16) |0x400);
		aml_write_reg32(P_VPP_MATRIX_COEF11_12, (0x1ea0 << 16) |0x1d25);
		aml_write_reg32(P_VPP_MATRIX_COEF20_21, (0x400 << 16) |0x717);
		aml_write_reg32(P_VPP_MATRIX_COEF22, 0x0);
		aml_write_reg32(P_VPP_MATRIX_OFFSET0_1, 0x0);
		aml_write_reg32(P_VPP_MATRIX_OFFSET2, 0x0);
	}
}

static void set_tcon_lvds(Lcd_Config_t *pConf)
{
	vpp_set_matrix_ycbcr2rgb(2, 0);
	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 3);
	aml_write_reg32(P_L_RGB_BASE_ADDR, 0);
	aml_write_reg32(P_L_RGB_COEFF_ADDR, 0x400);

	if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
	else if(pConf->lcd_basic.lcd_bits == 6)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);
	else
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0);

	aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
}



//new lvd_vx1_phy config
void lvds_phy_config(int lvds_vx1_sel)
{
	//debug
	aml_write_reg32(P_VPU_VLOCK_GCLK_EN, 7);
	aml_write_reg32(P_VPU_VLOCK_ADJ_EN_SYNC_CTRL, 0x108010ff);
	aml_write_reg32(P_VPU_VLOCK_CTRL, 0xe0f50f1b);
	//debug

	if(lvds_vx1_sel == 0){ //lvds
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL1, 0x6c6cca80);
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL2, 0x0000006c);
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL3, 0x0fff0800);
		//od   clk 1039.5 / 2 = 519.75 = 74.25*7
		aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL0, 0x0fff0040);
	}else{

	}

}

void vclk_set_encl_lvds(vmode_t vmode, int lvds_ports)
{
	int hdmi_clk_out;
	//int hdmi_vx1_clk_od1;
	int vx1_phy_div;
	int encl_div;
	unsigned int xd;
	//no used, od2 must >= od3.
	//hdmi_vx1_clk_od1 = 1; //OD1 always 1

	if(lvds_ports<2){
		//pll_video.pl 3500 pll_out
		switch(vmode) {
			case VMODE_LVDS_768P : //total: 1560x806 pixel clk = 75.5MHz, phy_clk(s)=(pclk*7)= 528.5 = 1057/2
				hdmi_clk_out = 1057;
				vx1_phy_div  = 2/2;
				encl_div     = vx1_phy_div*7;
				break;
			case VMODE_LVDS_1080P: //total: 2200x1125 pixel clk = 148.5MHz,phy_clk(s)=(pclk*7)= 1039.5 = 2079/2
				hdmi_clk_out = 2079;
				vx1_phy_div  = 2/2;
				encl_div     = vx1_phy_div*7;
				break;
			default:
				pr_err("Error video format!\n");
				return;
		}
		//if(set_hdmi_dpll(hdmi_clk_out,hdmi_vx1_clk_od1)) {
		if(set_hdmi_dpll(hdmi_clk_out,0)) {
			pr_err("Unsupported HDMI_DPLL out frequency!\n");
			return;
		}

		if(lvds_ports==1) //dual port
			vx1_phy_div = vx1_phy_div*2;
	}else if(lvds_ports>=2) {
		pr_err("Quad-LVDS is not supported!\n");
		return;
	}

	//configure vid_clk_div_top
	if((encl_div%14)==0){//7*even
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_14);
		xd = encl_div/14;
	}else if((encl_div%7)==0){ //7*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_7);
		xd = encl_div/7;
	}else{ //3.5*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_3p5);
		xd = encl_div/3.5;
	}
	//for lvds phy clock and enable decoupling FIFO
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL1,((3<<6)|((vx1_phy_div-1)<<1)|1)<<24);

	//config lvds phy
	lvds_phy_config(0);
	//configure crt_video
	set_crt_video_enc(0,0,xd);  //configure crt_video V1: inSel=vid_pll_clk(0),DivN=xd)
	enable_crt_video_encl(1,0); //select and enable the output
}

static void set_pll_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);

	vclk_set_encl_lvds(pDev->lcd_info.mode, pConf->lvds_mlvds_config.lvds_config->dual_port);
	aml_write_reg32( P_HHI_VIID_DIVIDER_CNTL, ((aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) & ~(0x7 << 8)) | (2 << 8) | (0<<10)) );
	aml_write_reg32(P_LVDS_GEN_CNTL, (aml_read_reg32(P_LVDS_GEN_CNTL)| (1 << 3) | (3<< 0)));
}

static void venc_set_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6TV
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (0<<0) |    // viu1 select encl
											(0<<2) );    // viu2 select encl
#endif
	aml_write_reg32(P_ENCL_VIDEO_EN, 0);
	//int havon_begin = 80;
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (0<<0) |    // viu1 select encl
											(0<<2) );     // viu2 select encl
	aml_write_reg32(P_ENCL_VIDEO_MODE, 0); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,     0x0418); // Sampling rate: 1

	// bypass filter
	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL, 0x1000);

	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT, pConf->lcd_basic.h_period - 1);
	if(cur_lvds_index)
		aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT, 1350 - 1);
	else
		aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT, pConf->lcd_basic.v_period - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN, pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END, pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,	pConf->lcd_timing.video_on_line);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,	pConf->lcd_basic.v_active - 1  + pConf->lcd_timing.video_on_line);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,	pConf->lcd_timing.sth1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,	pConf->lcd_timing.sth1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,	pConf->lcd_timing.stv1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,	pConf->lcd_timing.stv1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,	pConf->lcd_timing.stv1_vs_addr);//2);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,	pConf->lcd_timing.stv1_ve_addr);//4);

	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 0);

	// enable encl
	aml_write_reg32(P_ENCL_VIDEO_EN, 1);
}

static void set_control_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);

	if(lvds_repack_flag)
		lvds_repack = (pConf->lvds_mlvds_config.lvds_config->lvds_repack) & 0x1;
	pn_swap = (pConf->lvds_mlvds_config.lvds_config->pn_swap) & 0x1;
	dual_port = (pConf->lvds_mlvds_config.lvds_config->dual_port) & 0x1;
	if(port_reverse_flag)
		port_reverse = (pConf->lvds_mlvds_config.lvds_config->port_reverse) & 0x1;

	if(bit_num_flag){
		switch(pConf->lcd_basic.lcd_bits) {
			case 10: bit_num=0; break;
			case 8: bit_num=1; break;
			case 6: bit_num=2; break;
			case 4: bit_num=3; break;
			default: bit_num=1; break;
		}
	}

	aml_write_reg32(P_MLVDS_CONTROL,  (aml_read_reg32(P_MLVDS_CONTROL) & ~(1 << 0)));  //disable mlvds //有定义缺文档
	aml_write_reg32(P_LVDS_PACK_CNTL_ADDR,  //ok
					( lvds_repack<<0 ) | // repack
					( port_reverse?(0<<2):(1<<2)) | // odd_even
					( 0<<3 ) | // reserve
					( 0<<4 ) | // lsb first
					( pn_swap<<5 ) | // pn swap
					( dual_port<<6 ) | // dual port
					( 0<<7 ) | // use tcon control
					( bit_num<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
					( 0<<10 ) | //r_select  //0:R, 1:G, 2:B, 3:0
					( 1<<12 ) | //g_select  //0:R, 1:G, 2:B, 3:0
					( 2<<14 ));  //b_select  //0:R, 1:G, 2:B, 3:0; //ok
}

static void init_lvds_phy(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);
	aml_write_reg32( P_LVDS_SER_EN, 0xfff );
	aml_write_reg32( P_LVDS_PHY_CNTL0, 0x0002 );
	aml_write_reg32( P_LVDS_PHY_CNTL1, 0xff00 );
	aml_write_reg32( P_LVDS_PHY_CNTL3, 0x0ee1 );
	aml_write_reg32( P_LVDS_PHY_CNTL4, 0x3fff );
	aml_write_reg32( P_LVDS_PHY_CNTL5, 0xac24 );
}

static inline void _init_display_driver(Lcd_Config_t *pConf)
{
	int lcd_type;
	const char* lcd_type_table[]={
		"NULL",
		"TTL",
		"LVDS",
		"invalid",
	};

	lcd_type = pDev->conf.lcd_basic.lcd_type;
	printk("\nInit LCD type: %s.\n", lcd_type_table[lcd_type]);

	switch(lcd_type){
		case LCD_DIGITAL_LVDS:
			set_pll_lvds(pConf);
			venc_set_lvds(pConf);
			set_control_lvds(pConf);
			init_lvds_phy(pConf);
			set_tcon_lvds(pConf);
		break;

		default:
			pr_err("Invalid LCD type.\n");
		break;
	}
}

static inline void _disable_display_driver(void)
{
	int pll_sel, vclk_sel;

	pll_sel = 0;//((pConf->lcd_timing.clk_ctrl) >>12) & 0x1;
	vclk_sel = 0;//((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;

	aml_set_reg32_bits(P_HHI_VIID_DIVIDER_CNTL, 0, 11, 1);	//close lvds phy clk gate: 0x104c[11]

	aml_write_reg32(P_ENCT_VIDEO_EN, 0);	//disable enct
	aml_write_reg32(P_ENCL_VIDEO_EN, 0);	//disable encl

	if (vclk_sel)
		aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, 0, 0, 5);		//close vclk2 gate: 0x104b[4:0]
	else
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0, 0, 5);		//close vclk1 gate: 0x105f[4:0]
#if 0 //hsw
	if (pll_sel){
		aml_set_reg32_bits(P_HHI_VIID_DIVIDER_CNTL, 0, 16, 1);	//close vid2_pll gate: 0x104c[16]
		aml_set_reg32_bits(P_HHI_VIID_PLL_CNTL, 1, 30, 1);		//power down vid2_pll: 0x1047[30]
	}else{
		aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 16, 1);	//close vid1_pll gate: 0x1066[16]
		aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 0, 30, 1);		//power down vid1_pll: 0x105c[30]
		}
#endif
	pr_info("disable lcd display driver.\n");
}

static inline void _enable_vsync_interrupt(void)
{
	if ((aml_read_reg32(P_ENCT_VIDEO_EN) & 1) || (aml_read_reg32(P_ENCL_VIDEO_EN) & 1)) {
		aml_write_reg32(P_VENC_INTCTRL, 0x200);
	}else{
		aml_write_reg32(P_VENC_INTCTRL, 0x2);
	}
}
static void _lcd_module_enable(void)
{
	BUG_ON(pDev==NULL);
	_init_display_driver(&pDev->conf);
	_enable_vsync_interrupt();
}

static const vinfo_t *lcd_get_current_info(void)
{
	if(cur_lvds_index){
		pDev->lcd_info.name = "lvds1080p50hz";
		pDev->lcd_info.mode = VMODE_LVDS_1080P_50HZ;
		pDev->lcd_info.sync_duration_num = 50;
		pDev->lcd_info.sync_duration_den = 1;
	}else{
		pDev->lcd_info.name = "lvds1080p";
		pDev->lcd_info.mode = VMODE_LVDS_1080P;
		pDev->lcd_info.sync_duration_num = 60;
		pDev->lcd_info.sync_duration_den = 1;
	}
	return &pDev->lcd_info;
}

static int lcd_set_current_vmode(vmode_t mode)
{
	if ((mode != VMODE_LCD)&&(mode != VMODE_LVDS_1080P)&&(mode != VMODE_LVDS_1080P_50HZ))
		return -EINVAL;

	if(VMODE_LVDS_1080P_50HZ==(mode&VMODE_MODE_BIT_MASK))
		cur_lvds_index = 1;
	else
		cur_lvds_index = 0;

	aml_write_reg32(P_VPP_POSTBLEND_H_SIZE, pDev->lcd_info.width);
	_lcd_module_enable();

	if (VMODE_INIT_NULL == pDev->lcd_info.mode)
		pDev->lcd_info.mode = VMODE_LCD;

	return 0;
}

static vmode_t lcd_validate_vmode(char *mode)
{
	if ((strncmp(mode, "lvds1080p50hz", strlen("lvds1080p50hz"))) == 0)
		return VMODE_LVDS_1080P_50HZ;
	else if ((strncmp(mode, "lvds1080p", strlen("lvds1080p"))) == 0)
		return VMODE_LVDS_1080P;

	return VMODE_MAX;
}

static int lcd_vmode_is_supported(vmode_t mode)
{
	mode&=VMODE_MODE_BIT_MASK;

	if((mode == VMODE_LCD ) || (mode == VMODE_LVDS_1080P) || (mode == VMODE_LVDS_1080P_50HZ))
		return true;

	return false;
}

static int lcd_module_disable(vmode_t cur_vmod)
{
	return 0;
}

#ifdef  CONFIG_PM
static int lcd_suspend(void)
{
	BUG_ON(pDev==NULL);
	pr_info("lcd_suspend \n");

	_disable_display_driver();

	return 0;
}

static int lcd_resume(void)
{
	pr_info("lcd_resume\n");

	_lcd_module_enable();

	return 0;
}
#endif

static vout_server_t lcd_vout_server={
	.name = "lcd_vout_server",
	.op = {
		.get_vinfo = lcd_get_current_info,
		.set_vmode = lcd_set_current_vmode,
		.validate_vmode = lcd_validate_vmode,
		.vmode_is_supported=lcd_vmode_is_supported,
		.disable=lcd_module_disable,
#ifdef  CONFIG_PM
		.vout_suspend=lcd_suspend,
		.vout_resume=lcd_resume,
#endif
	},
};
static void _init_vout(lcd_dev_t *pDev)
{
	pDev->lcd_info.name = "lvds1080p";
	pDev->lcd_info.mode = VMODE_LVDS_1080P;
	pDev->lcd_info.width = pDev->conf.lcd_basic.h_active;
	pDev->lcd_info.height = pDev->conf.lcd_basic.v_active;
	pDev->lcd_info.field_height = pDev->conf.lcd_basic.v_active;
	pDev->lcd_info.aspect_ratio_num = pDev->conf.lcd_basic.screen_ratio_width;
	pDev->lcd_info.aspect_ratio_den = pDev->conf.lcd_basic.screen_ratio_height;
	pDev->lcd_info.screen_real_width= pDev->conf.lcd_basic.screen_actual_width;
	pDev->lcd_info.screen_real_height= pDev->conf.lcd_basic.screen_actual_height;
	pDev->lcd_info.sync_duration_num = pDev->conf.lcd_timing.sync_duration_num;
	pDev->lcd_info.sync_duration_den = pDev->conf.lcd_timing.sync_duration_den;

	vout_register_server(&lcd_vout_server);
}

static void _lcd_init(Lcd_Config_t *pConf)
{
	 _init_vout(pDev);
	 _lcd_module_enable();
}


static struct notifier_block lcd_reboot_nb;

static int lcd_reboot_notifier(struct notifier_block *nb, unsigned long state, void *cmd)
 {
	if (state == SYS_RESTART){
		pr_info("shut down lcd...\n");
	}

	return NOTIFY_DONE;
}


void panel_power_on(void)
{
	pr_info("%s\n", __func__);
}
void panel_power_off(void)
{
	pr_info("%s\n", __func__);
}
static ssize_t power_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	int ret = 0;
	bool status = false;
	int value;

	status = true;//gpio_get_status(PAD_GPIOZ_5);
	ret = sprintf(buf, "PAD_GPIOZ_5 gpio_get_status %s\n", (status?"true":"false"));
	ret = sprintf(buf, "\n");

	value = 0;//gpio_in_get(PAD_GPIOZ_5);
	ret = sprintf(buf, "PAD_GPIOZ_5 gpio_in_get %d\n", value);
	ret = sprintf(buf, "\n");

	return ret;
}
static ssize_t power_store(struct class *cls, struct class_attribute *attr,
			 const char *buf, size_t count)
{
	int ret = 0;
	int status = 0;
	status = simple_strtol(buf, NULL, 0);
	pr_info("input status %d\n", status);

	if (status != 0) {
		panel_power_off();
		pr_info("lvds_power_off\n");
	}else {
		panel_power_on();
		pr_info("lvds_power_on\n");
	}

	return ret;
}
static CLASS_ATTR(power, S_IWUSR | S_IRUGO, power_show, power_store);

static inline int _get_lcd_default_config(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int lvds_para[10];
	if (!pdev->dev.of_node){
		pr_err("\n can't get dev node---error----%s----%d",__FUNCTION__,__LINE__);
	}

	ret = of_property_read_u32_array(pdev->dev.of_node,"basic_setting",&lvds_para[0], 10);
	if(ret){
		pr_err("don't find to match basic_setting, use default setting.\n");
	}else{
		pr_info("get basic_setting ok.\n");
		pDev->conf.lcd_basic.h_active = lvds_para[0];
		pDev->conf.lcd_basic.v_active = lvds_para[1];
		pDev->conf.lcd_basic.h_period= lvds_para[2];
		pDev->conf.lcd_basic.v_period= lvds_para[3];
		pDev->conf.lcd_basic.screen_ratio_width = lvds_para[4];
		pDev->conf.lcd_basic.screen_ratio_height = lvds_para[5];
		pDev->conf.lcd_basic.screen_actual_width = lvds_para[6];
		pDev->conf.lcd_basic.screen_actual_height = lvds_para[7];
		pDev->conf.lcd_basic.lcd_type = lvds_para[8];
		pDev->conf.lcd_basic.lcd_bits = lvds_para[9];
	}
	ret = of_property_read_u32_array(pdev->dev.of_node,"delay_setting",&lvds_para[0], 8);
	if(ret){
		pr_err("don't find to match delay_setting, use default setting.\n");
	}else{
		pDev->conf.lcd_sequence.clock_enable_delay = lvds_para[0];
		pDev->conf.lcd_sequence.clock_disable_delay = lvds_para[1];
		pDev->conf.lcd_sequence.pwm_enable_delay= lvds_para[2];
		pDev->conf.lcd_sequence.pwm_disable_delay= lvds_para[3];
		pDev->conf.lcd_sequence.panel_power_on_delay = lvds_para[4];
		pDev->conf.lcd_sequence.panel_power_off_delay = lvds_para[5];
		pDev->conf.lcd_sequence.backlight_power_on_delay = lvds_para[6];
		pDev->conf.lcd_sequence.backlight_power_off_delay = lvds_para[7];
	}
	return ret;
}

// Define LVDS physical PREM SWING VCM REF
static Lvds_Phy_Control_t lcd_lvds_phy_control = {
	.lvds_prem_ctl  = 0x4,
	.lvds_swing_ctl = 0x2,
	.lvds_vcm_ctl   = 0x4,
	.lvds_ref_ctl   = 0x15,
	.lvds_phy_ctl0  = 0x0002,
	.lvds_fifo_wr_mode = 0x3,
};
//Define LVDS data mapping, pn swap.
static Lvds_Config_t lcd_lvds_config = {
	.lvds_repack    = 1,   //data mapping  //0:JEDIA mode, 1:VESA mode
	.pn_swap    = 0,       //0:normal, 1:swap
	.dual_port  = 1,
	.port_reverse   = 1,
};
static Lcd_Config_t m6tv_lvds_config = {
	// Refer to LCD Spec
	.lcd_basic = {
		.h_active = H_ACTIVE,
		.v_active = V_ACTIVE,
		.h_period = H_PERIOD,
		.v_period = V_PERIOD,
		.screen_ratio_width   = 16,
		.screen_ratio_height  = 9,
		.screen_actual_width  = 127, //this is value for 160 dpi please set real value according to spec.
		.screen_actual_height = 203, //
		.lcd_type = LCD_DIGITAL_LVDS,   //LCD_DIGITAL_TTL  //LCD_DIGITAL_LVDS  //LCD_DIGITAL_MINILVDS
		.lcd_bits = 8,  //8  //6
	},
	.lcd_timing = {
		.pll_ctrl = 0x40050c82,//0x400514d0, //
		.div_ctrl = 0x00010803,
		.clk_ctrl = 0x1111,  //[19:16]ss_ctrl, [12]pll_sel, [8]div_sel, [4]vclk_sel, [3:0]xd
		//.sync_duration_num = 501,
		//.sync_duration_den = 10,

		.video_on_pixel = VIDEO_ON_PIXEL,
		.video_on_line  = VIDEO_ON_LINE,

		.sth1_hs_addr = 44,
		.sth1_he_addr = 2156,
		.sth1_vs_addr = 0,
		.sth1_ve_addr = V_PERIOD - 1,
		.stv1_hs_addr = 2100,
		.stv1_he_addr = 2164,
		.stv1_vs_addr = 3,
		.stv1_ve_addr = 5,

		.pol_cntl_addr = (0x0 << LCD_CPH1_POL) |(0x0 << LCD_HS_POL) | (0x1 << LCD_VS_POL),
		.inv_cnt_addr = (0<<LCD_INV_EN) | (0<<LCD_INV_CNT),
		.tcon_misc_sel_addr = (1<<LCD_STV1_SEL) | (1<<LCD_STV2_SEL),
		.dual_port_cntl_addr = (1<<LCD_TTL_SEL) | (1<<LCD_ANALOG_SEL_CPH3) | (1<<LCD_ANALOG_3PHI_CLK_SEL) | (0<<LCD_RGB_SWP) | (0<<LCD_BIT_SWP),
	},
	.lcd_sequence = {
		.clock_enable_delay        = CLOCK_ENABLE_DELAY,
		.clock_disable_delay       = CLOCK_DISABLE_DELAY,
		.pwm_enable_delay          = PWM_ENABLE_DELAY,
		.pwm_disable_delay         = PWM_DISABLE_DELAY,
		.panel_power_on_delay      = PANEL_POWER_ON_DELAY,
		.panel_power_off_delay     = PANEL_POWER_OFF_DELAY,
		.backlight_power_on_delay  = BACKLIGHT_POWER_ON_DELAY,
		.backlight_power_off_delay = BACKLIGHT_POWER_OFF_DELAY,
	},
	.lvds_mlvds_config = {
		.lvds_config = &lcd_lvds_config,
		.lvds_phy_control = &lcd_lvds_phy_control,
	},
};

#ifdef CONFIG_USE_OF
static struct aml_lcd_platform m6tv_lvds_device = {
	.lcd_conf = &m6tv_lvds_config,
};
#define AMLOGIC_LVDS_DRV_DATA ((kernel_ulong_t)&m6tv_lvds_device)
static const struct of_device_id lvds_dt_match[]={
	{
		.compatible = "amlogic,lvds",
		.data = (void *)AMLOGIC_LVDS_DRV_DATA
	},
	{},
};
#else
#define lvds_dt_match NULL
#endif

#ifdef CONFIG_USE_OF
static inline struct aml_lcd_platform *lvds_get_driver_data(struct platform_device *pdev)
{
	const struct of_device_id *match;

	if(pdev->dev.of_node) {
		match = of_match_node(lvds_dt_match, pdev->dev.of_node);
		return (struct aml_lcd_platform *)match->data;
	}else
		pr_err("\n ERROR get data %d",__LINE__);
	return NULL;
}
#endif


static int lcd_probe(struct platform_device *pdev)
{
	struct aml_lcd_platform *pdata;
	int err;

#ifdef CONFIG_USE_OF
	pdata = lvds_get_driver_data(pdev);
#else
	pdata = pdev->dev.platform_data;
#endif

	pDev = (lcd_dev_t *)kmalloc(sizeof(lcd_dev_t), GFP_KERNEL);
	if (!pDev) {
		pr_err("[tcon]: Not enough memory.\n");
		return -ENOMEM;
	}

	pDev->conf = *(Lcd_Config_t *)(pdata->lcd_conf);        //*(Lcd_Config_t *)(s->start);

#ifdef CONFIG_USE_OF
	_get_lcd_default_config(pdev);
#endif

	printk("LCD probe ok\n");
	_lcd_init(&pDev->conf);
	lcd_reboot_nb.notifier_call = lcd_reboot_notifier;
	err = register_reboot_notifier(&lcd_reboot_nb);
	if (err) {
		pr_err("notifier register lcd_reboot_notifier fail!\n");
	}

	return 0;
}

static int lcd_remove(struct platform_device *pdev)
{
	unregister_reboot_notifier(&lcd_reboot_nb);
	kfree(pDev);

	return 0;
}

#ifdef CONFIG_PM
static int lcd_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int lcd_drv_resume(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	return 0;
}
#endif /* CONFIG_PM */

static struct platform_driver lcd_driver = {
	.probe = lcd_probe,
	.remove = lcd_remove,
	.driver = {
		.name = "mesonlcd",
		.of_match_table = lvds_dt_match,
	},
#ifdef CONFIG_PM
	.suspend = lcd_drv_suspend,
	.resume = lcd_drv_resume,
#endif
};

static int __init lcd_init(void)
{
	int ret;
	printk("TV LCD driver init\n");

	if (platform_driver_register(&lcd_driver)) {
		pr_err("failed to register tcon driver module\n");
		return -ENODEV;
	}

	aml_lcd_clsp = class_create(THIS_MODULE, "aml_lcd");
	ret = class_create_file(aml_lcd_clsp, &class_attr_power);

	return 0;
}

static void __exit lcd_exit(void)
{
	class_remove_file(aml_lcd_clsp, &class_attr_power);
	class_destroy(aml_lcd_clsp);
	platform_driver_unregister(&lcd_driver);
}

subsys_initcall(lcd_init);
module_exit(lcd_exit);

static  int __init lvds_boot_para_setup(char *s)
{
	unsigned char* ptr;
	unsigned char flag_buf[16];
	int i;

	pr_info("LVDS boot args: %s\n", s);

	if(strstr(s, "10bit")){
		bit_num_flag = 0;
		bit_num = 0;
	}
	if(strstr(s, "8bit")){
		bit_num_flag = 0;
		bit_num = 1;
	}
	if(strstr(s, "6bit")){
		bit_num_flag = 0;
		bit_num = 2;
	}
	if(strstr(s, "4bit")){
		bit_num_flag = 0;
		bit_num = 3;
	}
	if(strstr(s, "jeida")){
		lvds_repack_flag = 0;
		lvds_repack = 0;
	}
	if(strstr(s, "port_reverse")){
		port_reverse_flag = 0;
		port_reverse = 0;
	}
	if(strstr(s, "flaga")){
		i=0;
		ptr=strstr(s,"flaga")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flaga = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagb")){
		i=0;
		ptr=strstr(s,"flagb")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagb = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagc")){
		i=0;
		ptr=strstr(s,"flagc")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagc = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagd")){
		i=0;
		ptr=strstr(s,"flagd")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagd = simple_strtoul(flag_buf, NULL, 10);
	}
	return 0;
}
__setup("lvds=",lvds_boot_para_setup);

MODULE_DESCRIPTION("Meson LCD Panel Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic, Inc.");

