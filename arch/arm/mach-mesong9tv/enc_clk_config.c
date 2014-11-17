/*
 * arch/arm/mach-mesong9tv/enc_clk_config.c
 *
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/cpu.h>

#include <linux/clkdev.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <plat/io.h>
#include <plat/cpufreq.h>
#include <mach/am_regs.h>
#include <mach/clock.h>
#include <mach/cpu.h>

#include <linux/amlogic/vout/vinfo.h>
#include <linux/amlogic/vout/enc_clk_config.h>
#include "hw_enc_clk_config.h"

static DEFINE_MUTEX(enc_clock_lock);

#define check_clk_config(para)\
    if (para == -1)\
        return;

#define check_div() \
    if(div == -1)\
        return ;\
    switch(div){\
        case 1:\
            div = 0; break;\
        case 2:\
            div = 1; break;\
        case 4:\
            div = 2; break;\
        case 6:\
            div = 3; break;\
        case 12:\
            div = 4; break;\
        default:\
            break;\
    }

#define h_delay()       \
    do {                \
        int i = 1000;   \
        while(i--);     \
    }while(0)
static void hpll_load_en(void);
#define WAIT_FOR_PLL_LOCKED(reg)                        \
    do {                                                \
        unsigned int st = 0, cnt = 10;                  \
        while(cnt --) {                                 \
            aml_set_reg32_bits(reg, 0x5, 28, 3);        \
            aml_set_reg32_bits(reg, 0x4, 28, 3);        \
            hpll_load_en();                             \
            msleep_interruptible(10);                   \
            st = !!(aml_read_reg32(reg) & (1 << 31));   \
            if(st) {                                    \
                printk("hpll locked\n");                \
                break;                                  \
            }                                           \
            else {  /* reset pll */                     \
                printk("hpll reseting\n");              \
            }                                           \
        }                                               \
        if(cnt < 9)                                     \
            printk(KERN_CRIT "pll[0x%x] reset %d times\n", reg, 9 - cnt);\
    }while(0);

static void set_hpll_clk_out(unsigned clk)
{
    check_clk_config(clk);
    printk("config HPLL\n");
    aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x69c88000);
    aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0xca563823);
    aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x40238100);
    aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x00012286);
    aml_write_reg32(P_HHI_VID2_PLL_CNTL2, 0x430a800);       // internal LDO share with HPLL & VIID PLL
    switch(clk){
        case 2970:
            aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x59c84e00);
            aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0xce49c822);
            aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x4123b100);
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x00012385);

            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x6000043d);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x4000043d);
            printk("waiting HPLL lock\n");
            while(!(aml_read_reg32(P_HHI_VID_PLL_CNTL) & (1 << 31))) {
                ;
            }
            h_delay();
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x00016385);   // optimise HPLL VCO 2.97GHz performance
            break;
        case 2160:
            aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x59c80000);
            aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x0a563823);
            aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x0123b100);
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x12385);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x6001042d);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x4001042d);
            while(!(aml_read_reg32(P_HHI_VID_PLL_CNTL) & (1 << 31))) {
                ;
            }
            break;
        case 1488:
            aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x69c8ce00);
            aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x4023d100);
            aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x8a7ad023);
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x12286);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x6000043d);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x4000043d);
            while(!(aml_read_reg32(P_HHI_VID_PLL_CNTL) & (1 << 31))) {
                ;
            }
            break;
        case 1080:
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x6000042d);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x4000042d);
            break;
        case 1066:
            WRITE_CBUS_REG(HHI_VID_PLL_CNTL, 0x42a);
            break;
        case 1058:
            WRITE_CBUS_REG(HHI_VID_PLL_CNTL, 0x422);
            break;
        case 1086:
            WRITE_CBUS_REG(HHI_VID_PLL_CNTL, 0x43e);
            break;
        case 1296:
            aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x59c88000);
            aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0xca49b022);
            aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x0023b100);
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x00012385);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x600c0436);
            aml_write_reg32(P_HHI_VID_PLL_CNTL,  0x400c0436);
            aml_write_reg32(P_HHI_VID_PLL_CNTL5, 0x00016385);
            break;
        default:
            printk("error hpll clk: %d\n", clk);
            break;
    }
    if(clk < 2970)
        aml_write_reg32(P_HHI_VID_PLL_CNTL5, (aml_read_reg32(P_HHI_VID_PLL_CNTL5) & (~(0xf << 12))) | (0x6 << 12));
    printk("config HPLL done\n");
}

static void set_hpll_hdmi_od(unsigned div)
{
    check_clk_config(div);
    switch(div){
        case 1:
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 0, 18, 2);
            break;
        case 2:
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 1, 18, 2);
            break;
        case 3:
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 1, 16, 2);
            break;
        case 4:
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 3, 18, 2);
            break;
        case 8:
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 1, 16, 2);
            WRITE_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 3, 18, 2);
            break;
        default:
            break;
    }
}

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8
static void set_hpll_lvds_od(unsigned div)
{
    check_clk_config(div);
    switch(div) {
        case 1:
            aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 0, 16, 2);
            break;
        case 2:
            aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 1, 16, 2);
            break;
        case 4:
            aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 2, 16, 2);
            break;
        case 8:     // note: need test
            aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 3, 16, 2);
            break;
        default:
            break;
    }
}
#endif

// viu_channel_sel: 1 or 2
// viu_type_sel: 0: 0=ENCL, 1=ENCI, 2=ENCP, 3=ENCT.
int set_viu_path(unsigned viu_channel_sel, viu_type_e viu_type_sel)
{
    if((viu_channel_sel > 2) || (viu_channel_sel == 0))
        return -1;
    printk("VPU_VIU_VENC_MUX_CTRL: 0x%x\n", aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL));
    if(viu_channel_sel == 1){
        aml_set_reg32_bits(P_VPU_VIU_VENC_MUX_CTRL, viu_type_sel, 0, 2);
        printk("viu chan = 1\n");
    }
    else{
        //viu_channel_sel ==2
        aml_set_reg32_bits(P_VPU_VIU_VENC_MUX_CTRL, viu_type_sel, 2, 2);
        printk("viu chan = 2\n");
    }
    printk("VPU_VIU_VENC_MUX_CTRL: 0x%x\n", aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL));
    return 0;
}

static void set_vid_pll_div(unsigned div)
{
    check_clk_config(div);
    // Gate disable
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 16, 1);
    switch(div){
        case 10:
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 4, 4, 3);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 1, 8, 2);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 1, 12, 3);
            break;
        case 5:
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 4, 4, 3);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 8, 2);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 12, 3);
            break;
        case 6:
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 5, 4, 3);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 8, 2);
            aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 12, 3);
            break;
        default:
            break;
    }
    // Soft Reset div_post/div_pre
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 0, 2);
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 1, 3, 1);
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 1, 7, 1);
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 3, 0, 2);
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 3, 1);
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 7, 1);
    // Gate enable
    aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 1, 16, 1);
}

static void set_clk_final_div(unsigned div)
{
    check_clk_config(div);
    if(div == 0)
        div = 1;
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 1, 19, 1);
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 16, 3);
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_DIV, div-1, 0, 8);
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 7, 0, 3);
}

static void set_hdmi_tx_pixel_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_HDMI_CLK_CNTL, div, 16, 4);
}
static void set_encp_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_DIV, div, 24, 4);
}

static void set_enci_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_DIV, div, 28, 4);
}

static void set_enct_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VID_CLK_DIV, div, 20, 4);
}

static void set_encl_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VIID_CLK_DIV, div, 12, 4);
}

static void set_vdac0_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VIID_CLK_DIV, div, 28, 4);
}

static void set_vdac1_div(unsigned div)
{
    check_div();
    WRITE_CBUS_REG_BITS(HHI_VIID_CLK_DIV, div, 24, 4);
}

// --------------------------------------------------
//              clocks_set_vid_clk_div
// --------------------------------------------------
// wire            clk_final_en    = control[19];
// wire            clk_div1        = control[18];
// wire    [1:0]   clk_sel         = control[17:16];
// wire            set_preset      = control[15];
// wire    [14:0]  shift_preset    = control[14:0];
extern void clocks_set_vid_clk_div(int div_sel);
void clocks_set_vid_clk_div(int div_sel)
{
    int shift_val = 0;
    int shift_sel = 0;

    printk("%s[%d] div = %d\n", __func__, __LINE__, div_sel);
    // Disable the output clock
    aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 19, 1);
    aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 15, 1);

    switch(div_sel) {
    case CLK_UTIL_VID_PLL_DIV_1:      shift_val = 0xFFFF; shift_sel = 0; break;
    case CLK_UTIL_VID_PLL_DIV_2:      shift_val = 0x0aaa; shift_sel = 0; break;
    case CLK_UTIL_VID_PLL_DIV_3:      shift_val = 0x0db6; shift_sel = 0; break;
    case CLK_UTIL_VID_PLL_DIV_3p5:    shift_val = 0x36cc; shift_sel = 1; break;
    case CLK_UTIL_VID_PLL_DIV_3p75:   shift_val = 0x6666; shift_sel = 2; break;
    case CLK_UTIL_VID_PLL_DIV_4:      shift_val = 0x0ccc; shift_sel = 0; break;
    case CLK_UTIL_VID_PLL_DIV_5:      shift_val = 0x739c; shift_sel = 2; break;
    case CLK_UTIL_VID_PLL_DIV_6:      shift_val = 0x0e38; shift_sel = 0; break;
    case CLK_UTIL_VID_PLL_DIV_6p25:   shift_val = 0x0000; shift_sel = 3; break;
    case CLK_UTIL_VID_PLL_DIV_7:      shift_val = 0x3c78; shift_sel = 1; break;
    case CLK_UTIL_VID_PLL_DIV_7p5:    shift_val = 0x78f0; shift_sel = 2; break;
    case CLK_UTIL_VID_PLL_DIV_12:     shift_val = 0x0fc0; shift_sel = 0; break; 
    case CLK_UTIL_VID_PLL_DIV_14:     shift_val = 0x3f80; shift_sel = 1; break;
    case CLK_UTIL_VID_PLL_DIV_15:     shift_val = 0x7f80; shift_sel = 2; break;
    case CLK_UTIL_VID_PLL_DIV_2p5:    shift_val = 0x5294; shift_sel = 2; break;
    default: 
        printk("Error: clocks_set_vid_clk_div:  Invalid parameter\n");
        break;
    }

    if(shift_val == 0xffff ) {      // if divide by 1
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 1, 18, 1);
    } else {
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 16, 2);
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 15, 1);
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 0, 14);
        
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, shift_sel, 16, 2);
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 1, 15, 1);
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, shift_val, 0, 14);
        aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 0, 15, 1);
    }
    // Enable the final output clock
    aml_set_reg32_bits(P_HHI_VID_PLL_CLK_DIV, 1, 19, 1);
}

static void hpll_load_initial(void)
{
//hdmi load initial
    aml_write_reg32(P_HHI_VID_CLK_CNTL2, 0x2c);
    aml_write_reg32(P_HHI_VID_CLK_DIV, 0x100);
    aml_write_reg32(P_HHI_VIID_CLK_CNTL, 0x0);
    aml_write_reg32(P_HHI_VIID_CLK_DIV, 0x101);
    aml_write_reg32(P_HHI_VID_LOCK_CLK_CNTL, 0x80);

//    aml_write_reg32(P_HHI_VPU_CLK_CNTL, 25);
//    aml_set_reg32_bits(P_HHI_VPU_CLK_CNTL, 1, 8, 1);
    aml_write_reg32(P_AO_RTI_GEN_PWR_SLEEP0, 0x0);
    aml_write_reg32(P_HHI_HDMI_PLL_CNTL6, 0x100);

    aml_write_reg32(P_VPU_CLK_GATE, 0xffff);
    aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE, 0x0);
    aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN, 0x0);
    aml_write_reg32(P_VPU_VLOCK_GCLK_EN, 0x7);
    aml_write_reg32(P_VPU_VLOCK_ADJ_EN_SYNC_CTRL, 0x108010ff);
    aml_write_reg32(P_VPU_VLOCK_CTRL, 0xe0f50f1b);
}

static void hpll_load_en(void)
{
//hdmi load gen
    aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 1, 19, 1);
    aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 7, 0 , 3);
    aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 1, 16, 3);  // tmp use fclk_div4
    aml_write_reg32(P_ENCL_VIDEO_EN, 0x1);
//    msleep(20);
    aml_write_reg32(P_ENCL_VIDEO_EN, 0x0);
//    msleep(20);
//    printk("read Addr: 0x%x[0x%x]  Data: 0x%x\n", P_HHI_HDMI_PLL_CNTL, (P_HHI_HDMI_PLL_CNTL & 0xffff) >> 2, aml_read_reg32(P_HHI_HDMI_PLL_CNTL));
    aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0, 16, 3);  // use vid_pll
}

// mode hpll_clk_out hpll_hdmi_od viu_path viu_type vid_pll_div clk_final_div
// hdmi_tx_pixel_div unsigned encp_div unsigned enci_div unsigned enct_div unsigned ecnl_div;
static enc_clk_val_t setting_enc_clk_val[] = {
		{VMODE_480I,       2160, 8, 1, 1, VIU_ENCI,  5, 4, 2,-1,  2, -1, -1,  2,  -1},
		{VMODE_480I_RPT,   2160, 4, 1, 1, VIU_ENCI,  5, 4, 2,-1,  4, -1, -1,  2,  -1},
		{VMODE_480CVBS,    1296, 4, 1, 1, VIU_ENCI,  6, 4, 2,-1,  2, -1, -1,  2,  -1},
		{VMODE_480P,       2160, 8, 1, 1, VIU_ENCP,  5, 4, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_480P_RPT,   2160, 2, 1, 1, VIU_ENCP,  5, 4, 1, 2, -1, -1, -1,  1,  -1},
		{VMODE_576I,       2160, 8, 1, 1, VIU_ENCI,  5, 4, 2,-1,  2, -1, -1,  2,  -1},
		{VMODE_576I_RPT,   2160, 4, 1, 1, VIU_ENCI,  5, 4, 2,-1,  4, -1, -1,  2,  -1},
		{VMODE_576CVBS,    1296, 4, 1, 1, VIU_ENCI,  6, 4, 2,-1,  2, -1, -1,  2,  -1},
		{VMODE_576P,       2160, 8, 1, 1, VIU_ENCP,  5, 4, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_576P_RPT,   2160, 2, 1, 1, VIU_ENCP,  5, 4, 1, 2, -1, -1, -1,  1,  -1},
		{VMODE_720P,       1488, 2, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080I,      1488, 2, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080P,      1488, 1, 1, 1, VIU_ENCP, 10, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080P,      1488, 1, 1, 1, VIU_ENCP, 10, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_720P_50HZ,  1488, 2, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080I_50HZ, 1488, 2, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080P_50HZ, 1488, 1, 1, 1, VIU_ENCP, 10, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_1080P_24HZ, 1488, 2, 1, 1, VIU_ENCP, 10, 2, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_4K2K_30HZ,  2970, 1, 2, 1, VIU_ENCP,  5, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_4K2K_25HZ,  2970, 1, 2, 1, VIU_ENCP,  5, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_4K2K_24HZ,  2970, 1, 2, 1, VIU_ENCP,  5, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_4K2K_SMPTE, 2970, 1, 2, 1, VIU_ENCP,  5, 1, 1, 1, -1, -1, -1,  1,  -1},
		{VMODE_VGA,  1066, 3, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  1},
		{VMODE_SVGA, 1058, 2, 1, 1, VIU_ENCP, 10, 1, 2, 1, -1, -1, -1,  1,  1},
		{VMODE_XGA, 1085, 1, 1, 1, VIU_ENCP, 5, 1, 1, 1, -1, -1, -1,  1,  1},
};

#define MAX_CLK_INDEX   4
#define MAX_SAME_CONF   (4 + 1)
struct cts_mode_clk {
    vmode_t mode[MAX_SAME_CONF];
    struct {
        unsigned int cts_idx;
        unsigned char *name;
        unsigned int target_clk;
    }idx_clk[MAX_CLK_INDEX];
};

struct cts_mode_clk hdmitx_clk[] = {
    {
        .mode = {VMODE_1080P, VMODE_1080P_50HZ, VMODE_MAX},
        .idx_clk[0] = {
            .cts_idx = 55,
            .name = "VID_PLL_DIV_CLK_OUT",
            .target_clk = -1,//148000000,
        },
    },
    {
        .mode = {VMODE_4K2K_30HZ, VMODE_4K2K_25HZ, VMODE_4K2K_24HZ, VMODE_4K2K_SMPTE, VMODE_MAX},
        .idx_clk[0] = {
            .cts_idx = 55,
            .name = "VID_PLL_DIV_CLK_OUT",
            .target_clk = -1,//297000000 * 2,
        },
    },
    {
        .mode = {VMODE_4K2K_FAKE_5G, VMODE_4K2K_60HZ, VMODE_MAX},
        .idx_clk[0] = {
            .cts_idx = 55,
            .name = "VID_PLL_DIV_CLK_OUT",
            .target_clk = -1,//494000000,
        },
    },
};
/*
 * Please refer to clock tree document to check related clocks
 * 
 */
static unsigned int cts_clk_match(vmode_t mode)
{
    unsigned int i = 0, j = 0;
    unsigned int clk_msr = 0;

    for(i = 0; i < ARRAY_SIZE(hdmitx_clk); i ++) {
        for(j = 0; j < MAX_SAME_CONF; j ++) {
            if((mode == hdmitx_clk[i].mode[j]) && (hdmitx_clk[i].mode[j] != VMODE_MAX)) {
                clk_msr = clk_measure(hdmitx_clk[i].idx_clk[0].cts_idx);
                if(hdmitx_clk[i].idx_clk[0].target_clk == -1)
                    return 1;   // not check
                if(clk_msr == hdmitx_clk[i].idx_clk[0].target_clk) {
                    return 1;
                }
                else {
                    printk("mode: %d  %s(%d)  TargetClk: %d  !=  MsrClk: %d\n", mode, hdmitx_clk[i].idx_clk[0].name,
                           hdmitx_clk[i].idx_clk[0].cts_idx, hdmitx_clk[i].idx_clk[0].target_clk, clk_msr);
                    return 0;
                }
            }
        }
    }

    return 0;
}

#define MAX_CHK_TIMES   10
static void cts_clk_check(vmode_t mode)
{
    unsigned int chk_times = 0;

    while(chk_times < MAX_CHK_TIMES) {
        msleep_interruptible(100);
        if(cts_clk_match(mode))
            return;
        else {
            chk_times ++;
            WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
        }
    }
    if(chk_times == MAX_CHK_TIMES) {
        printk("**************************************************\n");
        printk("HDMI TX clock ABNORMAL STATE\n");
        printk("**************************************************\n");
    }
}

static int vmode_clk_match(vmode_t mode)
{
    unsigned int clk_msr = 0;

return 0;
    if(!(aml_read_reg32(P_HHI_HDMI_PLL_CNTL) & (1 << 31)))
        return 0;
    if((aml_read_reg32(P_HHI_HDMI_PLL_CNTL) & 0xfff) != 0x23d)
        return 0;
    if((mode == VMODE_4K2K_FAKE_5G) || (mode == VMODE_4K2K_5G))
        return 0;

    clk_msr = clk_measure(55);
    switch(mode) {
    case VMODE_720P:
    case VMODE_1080I:
    case VMODE_1080P:
    case VMODE_720P_50HZ:
    case VMODE_1080I_50HZ:
    case VMODE_1080P_50HZ:
    case VMODE_1080P_24HZ:
    case VMODE_4K2K_30HZ:
    case VMODE_4K2K_25HZ:
    case VMODE_4K2K_24HZ:
    case VMODE_4K2K_SMPTE:
    case VMODE_4K2K_60HZ:
        if(  (clk_msr == 148000000) || (clk_msr == 149000000)
          || (clk_msr == 593000000) || (clk_msr == 594000000) || (clk_msr == 595000000)
          || (clk_msr == 74000000) || (clk_msr == 73000000) || (clk_msr == 75000000))
            return 1;
        break;
    default:
        break;
    }
    return 0;
}

void set_vmode_clk(vmode_t mode)
{
    int i = 0;
    int j = 0;
    enc_clk_val_t *p_enc =NULL;

    hpll_load_initial();
printk("set_vmode_clk mode is %d\n", mode);

	if( (VMODE_576CVBS==mode) || (VMODE_480CVBS==mode) )
	{
		printk("g9tv: cvbs clk!\n");
		aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x5000022d);
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL2, 0x00890000);
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL3, 0x135c5091);
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
		// P_HHI_HDMI_PLL_CNTL5
		// 0x71c86900 for div2 disable inside PLL2 of HPLL
		// 0x71486900 for div2 enable inside PLL2 of HPLL
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL5, 0x71c86900);
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
	    aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x4000022d);

	    WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);

	    clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_5);

		// select vid_pll_clk for muxing
		aml_write_reg32(P_HHI_VID_CLK_CNTL, (aml_read_reg32(P_HHI_VID_CLK_CNTL)&(~(0x7<<16))) );
		// disable divider for clk_rst_tst()
		aml_write_reg32(P_HHI_VID_CLK_DIV, (aml_read_reg32(P_HHI_VID_CLK_DIV)&(~0xff)) );
		// select clk_div1 for enci clk muxing
		aml_write_reg32(P_HHI_VID_CLK_DIV, (aml_read_reg32(P_HHI_VID_CLK_DIV)&(~(0xf<<28))) );
		// select clk_div1 for vdac clk muxing
		aml_write_reg32(P_HHI_VIID_CLK_DIV, (aml_read_reg32(P_HHI_VIID_CLK_DIV)&(~(0x1<<19))) );
		aml_write_reg32(P_HHI_VIID_CLK_DIV, (aml_read_reg32(P_HHI_VIID_CLK_DIV)&(~(0xf<<28))) );
		// clk gate for enci(bit0) and vdac(bit4)
		aml_write_reg32(P_HHI_VID_CLK_CNTL2, (aml_read_reg32(P_HHI_VID_CLK_CNTL2)|0x1|(0x1<<4)) );

		return;
	}

    if(!vmode_clk_match(mode)) {
        printk("%s[%d] reset hdmi hpll\n", __func__, __LINE__);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x5000023d);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL2, 0x00454e00);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL3, 0x135c5091);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL4, 0x801da72c);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL5, 0x71486900);    //5940 0x71c86900      // 0x71486900 2970
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL6, 0x00000e55);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x4000023d);

        WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
    }
    aml_write_reg32(P_HHI_HDMI_CLK_CNTL, 0x100);

    switch(mode) {
    case VMODE_1080P:
    case VMODE_1080P_50HZ:
        aml_set_reg32_bits(P_HHI_HDMI_PLL_CNTL2, 0x44, 16, 8);
        break;
    case VMODE_4K2K_30HZ:
    case VMODE_4K2K_25HZ:
    case VMODE_4K2K_24HZ:
    case VMODE_4K2K_SMPTE:
        aml_set_reg32_bits(P_HHI_HDMI_PLL_CNTL2, 0, 16, 8);
        aml_set_reg32_bits(P_HHI_VID_CLK_DIV, 1, 0, 8);
        break;
    case VMODE_4K2K_60HZ:
        aml_set_reg32_bits(P_HHI_HDMI_PLL_CNTL2, 0, 16, 8);
        aml_set_reg32_bits(P_HHI_VID_CLK_DIV, 0, 0, 8);
        break;
    case VMODE_4K2K_FAKE_5G:
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x50000266);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x40000266);
        WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
        aml_set_reg32_bits(P_HHI_HDMI_PLL_CNTL2, 0x40, 16, 8);
        aml_set_reg32_bits(P_HHI_VID_CLK_DIV, 0, 0, 8);
        break;
    case VMODE_4K2K_5G:
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x50000266);
        aml_write_reg32(P_HHI_HDMI_PLL_CNTL, 0x40000266);
        WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL);
        aml_set_reg32_bits(P_HHI_HDMI_PLL_CNTL2, 0x44, 16, 8);
        aml_set_reg32_bits(P_HHI_VID_CLK_DIV, 1, 0, 8);
        break;
    default:
        break;
    }
    clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_5);
    cts_clk_check(mode);
return;

    p_enc=&setting_enc_clk_val[0];
    i = sizeof(setting_enc_clk_val) / sizeof(enc_clk_val_t);

    printk("mode is: %d\n", mode);
    for (j = 0; j < i; j++){
        if(mode == p_enc[j].mode)
            break;
    }
    set_viu_path(p_enc[j].viu_path, p_enc[j].viu_type);
    set_hpll_clk_out(p_enc[j].hpll_clk_out);
    set_hpll_lvds_od(p_enc[j].hpll_lvds_od);
    set_hpll_hdmi_od(p_enc[j].hpll_hdmi_od);
    set_vid_pll_div(p_enc[j].vid_pll_div);
    set_clk_final_div(p_enc[j].clk_final_div);
    set_hdmi_tx_pixel_div(p_enc[j].hdmi_tx_pixel_div);
    set_encp_div(p_enc[j].encp_div);
    set_enci_div(p_enc[j].enci_div);
    set_enct_div(p_enc[j].enct_div);
    set_encl_div(p_enc[j].encl_div);
    set_vdac0_div(p_enc[j].vdac0_div);
    set_vdac1_div(p_enc[j].vdac1_div);

}
 
