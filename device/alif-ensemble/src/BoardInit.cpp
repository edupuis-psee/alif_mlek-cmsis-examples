/*
 * SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BoardInit.hpp"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include "RTE_Components.h"
#include "RTE_Device.h"
#include CMSIS_device_header
#include "Driver_Common.h"
#include "ethosu_driver.h"
#include "uart_stdout.h"
#include "board.h"
#include "power.h"
#include <stdio.h>

static struct ethosu_driver npuDriver;
static void npu_irq_handler(void)
{
    ethosu_irq_handler(&npuDriver);
}

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

bool NpuInit()
{
    /* Base address is 0x4000E1000; interrupt number is 55. */
    void* const npuBaseAddr = reinterpret_cast<void*>(LOCAL_NPU_BASE);

    /*  Initialize Ethos-U NPU driver. */
    if (ethosu_init(&npuDriver, /* Arm Ethos-U device driver pointer  */
                    npuBaseAddr, /* Base address for the Arm Ethos-U device */
                    0, /* Cache memory pointer (not applicable for U55) */
                    0, /* Cache memory size */
                    1, /* Secure */
                    1) /* Privileged */ ) {
        printf("Failed to initialize Arm Ethos-U driver");
        return false;
    }

    NVIC_SetVector(LOCAL_NPU_IRQ_IRQn, (uint32_t) &npu_irq_handler);
    NVIC_EnableIRQ(LOCAL_NPU_IRQ_IRQn);

    return true;
}

/**
 * @brief  CPU L1-Cache enable.
 * @param  None
 * @retval None
 */
static void CpuCacheEnable(void)
{
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
}

#ifdef COPY_VECTORS
#include <string.h> // memcpy
static VECTOR_TABLE_Type MyVectorTable[496] __attribute__((aligned (2048))) __attribute__((section (".bss.noinit.ram_vectors")));
static void copy_vtor_table_to_ram()
{
    if (SCB->VTOR == (uint32_t) MyVectorTable) {
        return;
    }
    memcpy(MyVectorTable, (const void *) SCB->VTOR, sizeof MyVectorTable);
    __DMB();
    // Set the new vector table into use.
    SCB->VTOR = (uint32_t) MyVectorTable;
    __DSB();
}
#endif

void BoardInit(void)
{
#ifdef COPY_VECTORS
    copy_vtor_table_to_ram();
#endif
    BOARD_Pinmux_Init();

    /* Enable peripheral clocks */
    enable_cgu_clk38p4m();
    enable_cgu_clk160m();
    enable_cgu_clk100m();
    enable_cgu_clk20m();

    /* Enable MIPI power */
    enable_mipi_dphy_power();
    disable_mipi_dphy_isolation();

#if !defined(SEMIHOSTING)
    UartStdOutInit();
#endif /* defined(SEMIHOSTING) */

#if defined(ETHOSU_ARCH) && (ETHOSU_ARCH==u55)
    if (!NpuInit()) {
        return;
    }
#endif

    /* Enable the CPU Cache */
    CpuCacheEnable();
    return;
}
