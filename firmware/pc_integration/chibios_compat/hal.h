/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * HAL (Hardware Abstraction Layer) Compatibility for PC Emulator
 * Provides stub types that match ChibiOS HAL structures.
 */

#ifndef __HAL_H__
#define __HAL_H__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RTC Types
 * ============================================================================ */

#define HAL_USE_RTC 1

/**
 * @brief RTC time structure compatible with ChibiOS RTCTime.
 */
struct RTCTime {
    uint32_t tv_date;  // (year << 16) | (month << 8) | day
    uint32_t tv_time;  // (hour << 16) | (minute << 8) | second
};

/* ============================================================================
 * LPC43xx Register Stub Types
 * These are minimal stubs - not functional, just to satisfy includes
 * ============================================================================ */

typedef struct {
    volatile uint32_t RESERVED0;
    volatile uint32_t CREG0;
    volatile uint32_t RESERVED1[62];
    volatile uint32_t M4MEMMAP;
    volatile uint32_t RESERVED2[5];
    volatile uint32_t CREG5;
    volatile uint32_t RESERVED3[57];
    volatile uint32_t CHIPID;
    volatile uint32_t RESERVED4[65];
    volatile uint32_t M0SUBMEMMAP;
    volatile uint32_t RESERVED5[62];
    volatile uint32_t M0APPTXEVENT;
    volatile uint32_t M4TXEVENT;
    volatile uint32_t RESERVED6[62];
    volatile uint32_t USB0FLADJ;
    volatile uint32_t RESERVED7[63];
    volatile uint32_t USB1FLADJ;
} LPC_CREG_Type;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t STAT;
    volatile uint32_t MDIV;
    volatile uint32_t NP_DIV;
    volatile uint32_t FRAC;
} LPC_PLL_Type;

typedef struct {
    volatile uint32_t RESERVED0[5];
    volatile uint32_t FREQ_MON;
    volatile uint32_t XTAL_OSC_CTRL;
    volatile uint32_t PLL0USB_STAT;
    volatile uint32_t PLL0USB_CTRL;
    volatile uint32_t PLL0USB_MDIV;
    volatile uint32_t PLL0USB_NP_DIV;
    volatile uint32_t RESERVED1[1];
    volatile uint32_t PLL0AUDIO_STAT;
    volatile uint32_t PLL0AUDIO_CTRL;
    volatile uint32_t PLL0AUDIO_MDIV;
    volatile uint32_t PLL0AUDIO_NP_DIV;
    volatile uint32_t PLL0AUDIO_FRAC;
    volatile uint32_t PLL1_STAT;
    volatile uint32_t PLL1_CTRL;
    volatile uint32_t IDIVA_CTRL;
    volatile uint32_t IDIVB_CTRL;
    volatile uint32_t IDIVC_CTRL;
    volatile uint32_t IDIVD_CTRL;
    volatile uint32_t IDIVE_CTRL;
    volatile uint32_t BASE_SAFE_CLK;
    volatile uint32_t BASE_USB0_CLK;
    volatile uint32_t BASE_PERIPH_CLK;
    volatile uint32_t BASE_USB1_CLK;
    volatile uint32_t BASE_M4_CLK;
    volatile uint32_t BASE_SPIFI_CLK;
    volatile uint32_t BASE_SPI_CLK;
    volatile uint32_t BASE_PHY_RX_CLK;
    volatile uint32_t BASE_PHY_TX_CLK;
    volatile uint32_t BASE_APB1_CLK;
    volatile uint32_t BASE_APB3_CLK;
    volatile uint32_t BASE_LCD_CLK;
    volatile uint32_t BASE_ADCHS_CLK;
    volatile uint32_t BASE_SDIO_CLK;
    volatile uint32_t BASE_SSP0_CLK;
    volatile uint32_t BASE_SSP1_CLK;
    volatile uint32_t BASE_UART0_CLK;
    volatile uint32_t BASE_UART1_CLK;
    volatile uint32_t BASE_UART2_CLK;
    volatile uint32_t BASE_UART3_CLK;
    volatile uint32_t BASE_OUT_CLK;
    volatile uint32_t RESERVED2[4];
    volatile uint32_t BASE_AUDIO_CLK;
    volatile uint32_t BASE_CGU_OUT0_CLK;
    volatile uint32_t BASE_CGU_OUT1_CLK;
} LPC_CGU_Type;

typedef struct {
    volatile uint32_t PM;
    volatile uint32_t BASE_STAT;
    volatile uint32_t RESERVED0[62];
    volatile uint32_t CLK_APB3_BUS_CFG;
    volatile uint32_t CLK_APB3_BUS_STAT;
    // ... (simplified - just need offsets to be correct)
    volatile uint32_t RESERVED1[700];
    volatile uint32_t CLK_ADCHS_CFG;
    volatile uint32_t CLK_ADCHS_STAT;
} LPC_CCU1_Type;

typedef struct {
    volatile uint32_t RESERVED0[64];
    volatile uint32_t RESET_CTRL[2];
    volatile uint32_t RESERVED1[2];
    volatile uint32_t RESET_STATUS[4];
    volatile uint32_t RESERVED2[12];
    volatile uint32_t RESET_ACTIVE_STATUS[2];
    volatile uint32_t RESERVED3[170];
    volatile uint32_t RESET_EXT_STAT[64];
} LPC_RGU_Type;

typedef struct {
    volatile uint32_t SFSP[16][32];
    volatile uint32_t RESERVED0[256];
    volatile uint32_t SFSCLK[4];
    volatile uint32_t RESERVED1[28];
    volatile uint32_t SFSUSB;
    volatile uint32_t SFSI2C0;
    volatile uint32_t ENAIO[3];
    volatile uint32_t RESERVED2[27];
    volatile uint32_t EMCDELAYCLK;
    volatile uint32_t RESERVED3[63];
    volatile uint32_t PINTSEL0;
    volatile uint32_t PINTSEL1;
} LPC_SCU_Type;

typedef struct {
    volatile uint32_t OUT_MUX_CFG[16];
    volatile uint32_t SGPIO_MUX_CFG[16];
    volatile uint32_t SLICE_MUX_CFG[16];
    volatile uint32_t REG[16];
    volatile uint32_t REG_SS[16];
    volatile uint32_t PRESET[16];
    volatile uint32_t COUNT[16];
    volatile uint32_t POS[16];
    volatile uint32_t MASK_A;
    volatile uint32_t MASK_H;
    volatile uint32_t MASK_I;
    volatile uint32_t MASK_P;
    volatile uint32_t GPIO_INREG;
    volatile uint32_t GPIO_OUTREG;
    volatile uint32_t GPIO_OENREG;
    volatile uint32_t CTRL_ENABLE;
    volatile uint32_t CTRL_DISABLE;
    volatile uint32_t RESERVED0[823];
    volatile uint32_t CLR_EN_0;
    volatile uint32_t SET_EN_0;
    volatile uint32_t ENABLE_0;
    volatile uint32_t STATUS_0;
    volatile uint32_t CTR_STATUS_0;
    volatile uint32_t SET_STATUS_0;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t CLR_EN_1;
    volatile uint32_t SET_EN_1;
    volatile uint32_t ENABLE_1;
    volatile uint32_t STATUS_1;
    volatile uint32_t CTR_STATUS_1;
    volatile uint32_t SET_STATUS_1;
    volatile uint32_t RESERVED2[2];
    volatile uint32_t CLR_EN_2;
    volatile uint32_t SET_EN_2;
    volatile uint32_t ENABLE_2;
    volatile uint32_t STATUS_2;
    volatile uint32_t CTR_STATUS_2;
    volatile uint32_t SET_STATUS_2;
    volatile uint32_t RESERVED3[2];
    volatile uint32_t CLR_EN_3;
    volatile uint32_t SET_EN_3;
    volatile uint32_t ENABLE_3;
    volatile uint32_t STATUS_3;
    volatile uint32_t CTR_STATUS_3;
    volatile uint32_t SET_STATUS_3;
} LPC_SGPIO_Type;

typedef struct {
    volatile uint32_t SRCADDR;
    volatile uint32_t DESTADDR;
    volatile uint32_t LLI;
    volatile uint32_t CONTROL;
    volatile uint32_t CONFIG;
    volatile uint32_t RESERVED[3];
} GPDMA_CH_Type;

typedef struct {
    volatile uint32_t INTSTAT;
    volatile uint32_t INTTCSTAT;
    volatile uint32_t INTTCCLEAR;
    volatile uint32_t INTERRSTAT;
    volatile uint32_t INTERRCLR;
    volatile uint32_t RAWINTTCSTAT;
    volatile uint32_t RAWINTERRSTAT;
    volatile uint32_t ENBLDCHNS;
    volatile uint32_t SOFTBREQ;
    volatile uint32_t SOFTSREQ;
    volatile uint32_t SOFTLBREQ;
    volatile uint32_t SOFTLSREQ;
    volatile uint32_t CONFIG;
    volatile uint32_t SYNC;
    volatile uint32_t RESERVED[50];
    GPDMA_CH_Type CH[8];
} LPC_GPDMA_Type;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t PWRCTRL;
    volatile uint32_t CLKDIV;
    volatile uint32_t CLKSRC;
    volatile uint32_t CLKENA;
    volatile uint32_t TMOUT;
    volatile uint32_t CTYPE;
    volatile uint32_t BLKSIZ;
    volatile uint32_t BYTCNT;
    volatile uint32_t INTMASK;
    volatile uint32_t CMDARG;
    volatile uint32_t CMD;
    volatile uint32_t RESP0;
    volatile uint32_t RESP1;
    volatile uint32_t RESP2;
    volatile uint32_t RESP3;
    volatile uint32_t MINTSTS;
    volatile uint32_t RINTSTS;
    volatile uint32_t STATUS;
    volatile uint32_t FIFOTH;
    volatile uint32_t CDETECT;
    volatile uint32_t WRTPRT;
    volatile uint32_t GPIO;
    volatile uint32_t TCBCNT;
    volatile uint32_t TBBCNT;
    volatile uint32_t DEBNCE;
    volatile uint32_t USRID;
    volatile uint32_t VERID;
    volatile uint32_t RESERVED0;
    volatile uint32_t UHS_REG;
    volatile uint32_t RST_N;
    volatile uint32_t RESERVED1;
    volatile uint32_t BMOD;
    volatile uint32_t PLDMND;
    volatile uint32_t DBADDR;
    volatile uint32_t IDSTS;
    volatile uint32_t IDINTEN;
    volatile uint32_t DSCADDR;
    volatile uint32_t BUFADDR;
    volatile uint32_t RESERVED2[25];
    volatile uint32_t DATA;
} LPC_SDMMC_Type;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t CMD;
    volatile uint32_t ADDR;
    volatile uint32_t IDATA;
    volatile uint32_t CLIMIT;
    volatile uint32_t DATA;
    volatile uint32_t MCMD;
    volatile uint32_t STAT;
} LPC_SPIFI_Type;

typedef struct {
    volatile uint32_t IR;
    volatile uint32_t TCR;
    volatile uint32_t TC;
    volatile uint32_t PR;
    volatile uint32_t PC;
    volatile uint32_t MCR;
    volatile uint32_t MR[4];
    volatile uint32_t CCR;
    volatile uint32_t CR[4];
    volatile uint32_t EMR;
    volatile uint32_t RESERVED0[12];
    volatile uint32_t CTCR;
} LPC_TIMER_Type;

typedef struct {
    volatile uint32_t ILR;
    volatile uint32_t CCR;
    volatile uint32_t CIIR;
    volatile uint32_t AMR;
    volatile uint32_t CTIME0;
    volatile uint32_t CTIME1;
    volatile uint32_t CTIME2;
    volatile uint32_t SEC;
    volatile uint32_t MIN;
    volatile uint32_t HOUR;
    volatile uint32_t DOM;
    volatile uint32_t DOW;
    volatile uint32_t DOY;
    volatile uint32_t MONTH;
    volatile uint32_t YEAR;
    volatile uint32_t CALIBRATION;
    volatile uint32_t RESERVED0[7];
    volatile uint32_t ASEC;
    volatile uint32_t AMIN;
    volatile uint32_t AHOUR;
    volatile uint32_t ADOM;
    volatile uint32_t ADOW;
    volatile uint32_t ADOY;
    volatile uint32_t AMON;
    volatile uint32_t AYEAR;
} LPC_RTC_Type;

/* Global register pointers - these would be memory-mapped on real hardware */
/* For PC emulator, these are just static instances */
extern LPC_CREG_Type* LPC_CREG;
extern LPC_CGU_Type* LPC_CGU;
extern LPC_CCU1_Type* LPC_CCU1;
extern LPC_RGU_Type* LPC_RGU;
extern LPC_SCU_Type* LPC_SCU;
extern LPC_SGPIO_Type* LPC_SGPIO;
extern LPC_GPDMA_Type* LPC_GPDMA;
extern LPC_SDMMC_Type* LPC_SDMMC;
extern LPC_SPIFI_Type* LPC_SPIFI;
extern LPC_RTC_Type* LPC_RTC;

/* ARM intrinsics stubs */
static inline uint32_t __get_APSR(void) { return 0; }
static inline void __SEV(void) {}

/* NVIC stubs */
#define CORTEX_PRIORITY_MASK(x) (x)

typedef int IRQn_Type;
#define M4CORE_IRQn 0
#define M0CORE_IRQn 1

static inline void nvicEnableVector(IRQn_Type irq, uint32_t prio) {
    (void)irq; (void)prio;
}

static inline void nvicDisableVector(IRQn_Type irq) {
    (void)irq;
}

/* LPC43xx IRQ priorities */
#define LPC43XX_M4TXEVENT_IRQ_PRIORITY 0
#define LPC43XX_M0APPTXEVENT_IRQ_PRIORITY 0

#ifdef __cplusplus
}
#endif

#endif /* PORTAPACK_PC_EMULATOR */

#endif /* __HAL_H__ */
