#pragma once

#include "util.h"

#define BAT_CTRL_REG  MMIO16(0xFF4812)
#define BAT_CTRL2_REG MMIO16(0xFF4814)
#define CP_CTRL_REG   MMIO16(0xFF481C)
#define PAD_CTRL_REG  MMIO16(0xFF481E)
#define LED_CTRL_REG  MMIO16(0xFF4820)

#define CP_CTRL_REG_CP_EN(n)      ((n) << 0)
#define CP_CTRL_REG_CP_LEVEL1(n)  ((n) << 2)
#define CP_CTRL_REG_CP_LEVEL2(n)  ((n) << 4)
#define CP_CTRL_REG_CP_PWM1       (1 << 6)
#define CP_CTRL_REG_CP_PWM2       (1 << 7)
#define CP_CTRL_REG_CP_FREQ(n)    ((n) << 8)
#define CP_CTRL_REG_CP_PARALLEL   (1 << 10)
#define CP_CTRL_REG_CP_MODE       (1 << 11)

#define LED_CTRL_REG_WLED_CUR0(n)  ((n) << 0)
#define LED_CTRL_REG_WLED_CUR1(n)  ((n) << 2)
#define LED_CTRL_REG_WLED_CUR2(n)  ((n) << 4)
#define LED_CTRL_REG_WLED0_PWM     (1 << 6)
#define LED_CTRL_REG_WLED1_PWM     (1 << 7)
#define LED_CTRL_REG_WLED2_PWM     (1 << 8)
#define LED_CTRL_REG_WLED_PWM_CTRL (1 << 9)
