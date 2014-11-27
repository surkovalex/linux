/*
 * Color Management
 *
 * Author: dezhi kong <dezhi.kong@amlogic.com>
 *
 * Copyright (C) 2014 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AMVECM_VLOCK_REGMAP_H
#define __AMVECM_VLOCK_REGMAP_H

#include <linux/amlogic/cm.h>
#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV)
//TV_ENC_LCD720x480
static am_regs_t vlock_enc={
	20,
	{
/* optimize */
{REG_TYPE_VCBUS, 0x3000,     0xffffffff, 0xE0f50f1b  },
{REG_TYPE_VCBUS, 0x3001,     0xffffffff, 0x41E3c3c   },
{REG_TYPE_VCBUS, 0x3002,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3003,     0xffffffff, 0x40280280  },
{REG_TYPE_VCBUS, 0x3004,     0xffffffff, 0x280280    },
{REG_TYPE_VCBUS, 0x3005,     0xffffffff, 0x8020000   },
{REG_TYPE_VCBUS, 0x3006,     0xffffffff, 0x0008000   },
{REG_TYPE_VCBUS, 0x3007,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3008,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3009,     0xffffffff, 0x0008000   },
{REG_TYPE_VCBUS, 0x300a,     0xffffffff, 0x8000000   },
{REG_TYPE_VCBUS, 0x300b,     0xffffffff, 0x0008000   },
{REG_TYPE_VCBUS, 0x300c,     0xffffffff, 0xa000000   },
{REG_TYPE_VCBUS, 0x300d,     0xffffffff, 0x0004000   },
{REG_TYPE_VCBUS, 0x3010,     0xffffffff, 0x20001000  },
{REG_TYPE_VCBUS, 0x3016,     0xffffffff, 0x18000     },
{REG_TYPE_VCBUS, 0x3017,     0xffffffff, 0x01080     },
{REG_TYPE_VCBUS, 0x301d,     0xffffffff, 0x30501080  },
{REG_TYPE_VCBUS, 0x301e,     0xffffffff, 0x7	    },
{REG_TYPE_VCBUS, 0x301f,     0xffffffff, 0x6000000   },
{0}
	}
};
static am_regs_t vlock_pll={
	20,
	{
/* optimize */
{REG_TYPE_VCBUS,	0x3000,	    0xffffffff,	0xE0f50f1b  },
{REG_TYPE_VCBUS, 0x3001,     0xffffffff, 0x4013c3c   },
{REG_TYPE_VCBUS, 0x3002,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3003,     0xffffffff, 0x40240280  },
{REG_TYPE_VCBUS, 0x3004,     0xffffffff, 0x240280    },
{REG_TYPE_VCBUS, 0x3005,     0xffffffff, 0x8020000   },
{REG_TYPE_VCBUS, 0x3006,     0xffffffff, 0x0100000   },
{REG_TYPE_VCBUS, 0x3007,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3008,     0xffffffff, 0x6000000   },
{REG_TYPE_VCBUS, 0x3009,     0xffffffff, 0x0100000   },
{REG_TYPE_VCBUS, 0x300a,     0xffffffff, 0x8000000   },
{REG_TYPE_VCBUS, 0x300b,     0xffffffff, 0x0100000   },
{REG_TYPE_VCBUS, 0x300c,     0xffffffff, 0x0100000   },
{REG_TYPE_VCBUS, 0x300d,     0xffffffff, 0x0000100   },
{REG_TYPE_VCBUS, 0x3010,     0xffffffff, 0x20001000  },
{REG_TYPE_VCBUS, 0x3016,     0xffffffff, 0x81eff     },
{REG_TYPE_VCBUS, 0x3017,     0xffffffff, 0x01080     },
{REG_TYPE_VCBUS, 0x301d,     0xffffffff, 0x30501080  },
{REG_TYPE_VCBUS, 0x301e,     0xffffffff, 0x7	    },
{REG_TYPE_VCBUS, 0x301f,     0xffffffff, 0x6000000   },
{0}
	}
};
#endif

#endif

