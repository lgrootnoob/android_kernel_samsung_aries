/* arch/arm/mach-s5pv210/include/mach/voltages.h
*
* Copyright (c) 2010 Samsung Electronics Co., Ltd.
*
* S5PV210/S5PC110 CPU frequency scaling support
*
* Aries and P1 devices
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_VOLTAGES_H
#define __ASM_ARCH_VOLTAGES_H

#if defined(CONFIG_SAMSUNG_GALAXYS4G)

#define ARMVOLTMAX 1450000
#define INTVOLTMAX 1250000

#else

#define ARMVOLTMAX 1500000
#define INTVOLTMAX 1300000

#endif

#define ARMBOOT 1250000
#define INTBOOT 1125000

#if defined(CONFIG_SAMSUNG_GALAXYS4G)

// DVS ARM
#define DVSARM0 1425000 // 1400 MHz
#define DVSARM1 1400000 // 1300 MHz
#define DVSARM2 1375000 // 1200 MHz
#define DVSARM3 1350000 // 1100 MHz
#define DVSARM4 1325000 // 1000 MHz
#define DVSARM5 1250000 //  800 MHz

#elif defined(CONFIG_MACH_ARIES)

// DVS ARM
#define DVSARM0 1450000 // 1400 MHz
#define DVSARM1 1400000 // 1300 MHz
#define DVSARM2 1350000 // 1200 MHz
#define DVSARM3 1300000 // 1100 MHz
#define DVSARM4 1275000 // 1000 MHz
#define DVSARM5 1200000 //  800 MHz

#else // CONFIG_MACH_P1

// DVS ARM
#define DVSARM0 1500000 // 1400 MHz
#define DVSARM1 1475000 // 1300 MHz
#define DVSARM2 1450000 // 1200 MHz
#define DVSARM3 1425000 // 1100 MHz
#define DVSARM4 1375000 // 1000 MHz
#define DVSARM5 1300000 //  800 MHz

#endif

#ifdef CONFIG_SAMSUNG_GALAXYS4G

// DVS ARM
#define DVSARM6 1100000 // 400 MHz
#define DVSARM7 1000000 // 100, 200 MHz

#else

// Default values for Aries and P1 devices
// DVS ARM
#define DVSARM6 1050000 // 400 MHz
#define DVSARM7  950000 // 100, 200 MHz

#endif

// DVS INT
#define DVSINT0 1200000 // 1300, 1400 MHz
#define DVSINT1 1125000 // 1100, 1200 MHz
#define DVSINT2 1100000 // 200, 400, 800, 1000 MHz
#define DVSINT3 1000000 // 100 MHz

#endif /* __ASM_ARCH_VOLTAGES_H */
