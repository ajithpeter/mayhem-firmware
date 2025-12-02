/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * HAL Compatibility Implementation for PC Emulator
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "hal.h"

// Static instances of LPC43xx register structures
static LPC_CREG_Type lpc_creg_instance = {};
static LPC_CGU_Type lpc_cgu_instance = {};
static LPC_CCU1_Type lpc_ccu1_instance = {};
static LPC_RGU_Type lpc_rgu_instance = {};
static LPC_SCU_Type lpc_scu_instance = {};
static LPC_SGPIO_Type lpc_sgpio_instance = {};
static LPC_GPDMA_Type lpc_gpdma_instance = {};
static LPC_SDMMC_Type lpc_sdmmc_instance = {};
static LPC_SPIFI_Type lpc_spifi_instance = {};
static LPC_RTC_Type lpc_rtc_instance = {};

// Global pointers to register structures
LPC_CREG_Type* LPC_CREG = &lpc_creg_instance;
LPC_CGU_Type* LPC_CGU = &lpc_cgu_instance;
LPC_CCU1_Type* LPC_CCU1 = &lpc_ccu1_instance;
LPC_RGU_Type* LPC_RGU = &lpc_rgu_instance;
LPC_SCU_Type* LPC_SCU = &lpc_scu_instance;
LPC_SGPIO_Type* LPC_SGPIO = &lpc_sgpio_instance;
LPC_GPDMA_Type* LPC_GPDMA = &lpc_gpdma_instance;
LPC_SDMMC_Type* LPC_SDMMC = &lpc_sdmmc_instance;
LPC_SPIFI_Type* LPC_SPIFI = &lpc_spifi_instance;
LPC_RTC_Type* LPC_RTC = &lpc_rtc_instance;

#endif /* PORTAPACK_PC_EMULATOR */
