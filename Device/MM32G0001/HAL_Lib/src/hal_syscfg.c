/***********************************************************************************************************************
    @file     hal_syscfg.c
    @author   VV TEAM
    @brief    THIS FILE PROVIDES ALL THE SYSCFG FIRMWARE FUNCTIONS.
  **********************************************************************************************************************
    @attention

    <h2><center>&copy; Copyright(c) <2023> <MindMotion></center></h2>

      Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
    following conditions are met:
    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
       the following disclaimer in the documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *********************************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "hal_syscfg.h"

/** @addtogroup MM32_StdPeriph_Driver
  * @{
  */

/** @defgroup SYSCFG
  * @{
  */

/** @defgroup SYSCFG_Private_Defines
  * @{
  */

/**
  * @}
  */

/** @defgroup SYSCFG_Private_Functions
  * @{
  */

/**
  * @brief  Deinitializes the SYSCFG registers to their default reset values.
  * @param  None
  * @retval None
  */
void SYSCFG_DeInit(void)
{
     /* Set SYSCFG_CFGR1 register to reset value without affecting MEM_MODE bits */
    SYSCFG->CFGR &= SYSCFG_CFGR_MEM_MODE_Msk;
     /* Set EXTICRx registers to reset value */
    SYSCFG->EXTICR1 = 0;
    SYSCFG->EXTICR2 = 0;
    SYSCFG->EXTICR3 = 0;
    SYSCFG->EXTICR4 = 0;
}

/**
  * @brief  Configures the memory mapping at address 0x00000000.
  * @param  SYSCFG_MemoryRemap: selects the memory remapping.
  *          This parameter can be one of the following values:
  *            @arg SYSCFG_MEMORYREMAP_FLASH : Main Flash memory mapped at 0x00000000
  *            @arg SYSCFG_MEMORYREMAP_SYSTEMMEMORY : System Flash memory mapped at 0x00000000
  *            @arg SYSCFG_MEMORYREMAP_SRAM : Embedded SRAM mapped at 0x00000000
  * @retval None
  */
void SYSCFG_MemoryRemapConfig(uint32_t SYSCFG_MemoryRemap)
{
    MODIFY_REG(SYSCFG->CFGR, SYSCFG_CFGR_MEM_MODE_Msk, SYSCFG_MemoryRemap);
}

/**
  * @brief  Selects the GPIO pin used as EXTI Line.
  * @param  port_source_gpio: EXTI_PORTSOURCEGPIOx .
  * @param  pin_source: EXTI_PINSOURCEx.
  * @retval None.
  */
void SYSCFG_EXTILineConfig(uint8_t port_source_gpio, uint8_t pin_source)
{
    uint32_t tmp = 0x00;

    tmp = ((uint32_t)0x0F) << (0x04 * (pin_source & (uint8_t)0x03));

    if ((pin_source >> 0x02) == 0)
    {
        SYSCFG->EXTICR1 &= ~tmp;
        SYSCFG->EXTICR1 |= (((uint32_t)port_source_gpio) << (0x04 * (pin_source & (uint8_t)0x03)));
    }
    else if ((pin_source >> 0x02) == 1)
    {
        SYSCFG->EXTICR2 &= ~tmp;
        SYSCFG->EXTICR2 |= (((uint32_t)port_source_gpio) << (0x04 * (pin_source & (uint8_t)0x03)));
    }
    else if ((pin_source >> 0x02) == 2)
    {
        SYSCFG->EXTICR3 &= ~tmp;
        SYSCFG->EXTICR3 |= (((uint32_t)port_source_gpio) << (0x04 * (pin_source & (uint8_t)0x03)));
    }
    else if ((pin_source >> 0x02) == 3)
    {
        SYSCFG->EXTICR4 &= ~tmp;
        SYSCFG->EXTICR4 |= (((uint32_t)port_source_gpio) << (0x04 * (pin_source & (uint8_t)0x03)));
    }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

