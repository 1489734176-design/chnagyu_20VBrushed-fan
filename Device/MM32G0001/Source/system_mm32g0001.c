/***********************************************************************************************************************
    @file     system_mm32g0001.c
    @author   VV TEAM
    @brief    THIS FILE PROVIDES ALL THE SYSTEM FUNCTIONS.
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

/* Files includes ----------------------------------------------------------------------------------------------------*/
#include "mm32_device.h"

/*
 *  Only one of SYSCLK_HSI_XXMHz and SYSCLK_HSE_XXMHz can be defined at a time.
 *  When HSI(HSI_VALUE =  48MHz) is used as the clock source, SYSCLK_HSI_XXMHz is used.
 *  When HSE is used as the clock source, SYSCLK_HSE_XXMHz is used.
 */

#define SYSCLK_HSI_XXMHz  48000000     /*Do not change this value if hsi is used as the system clock */
// #define SYSCLK_HSE_XXMHz    HSE_VALUE  /*Modify HSE_VALUE in mm32g0001.h */

/* Clock Definitions -------------------------------------------------------------------------------------------------*/
/* System Clock Frequency Select */
#if defined SYSCLK_HSE_XXMHz
uint32_t SystemCoreClock         = SYSCLK_HSE_XXMHz; /*!< HSE Selected as System Clock source */

#elif defined SYSCLK_HSI_XXMHz
uint32_t SystemCoreClock         = SYSCLK_HSI_XXMHz; /*!< HSI Selected as System Clock source */
#endif

#define RCC_CFGR_HPRE_DIV1              (0x00U << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV2              (0x08U << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV4              (0x09U << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV8              (0x0AU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV16             (0x0BU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV64             (0x0CU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV128            (0x0DU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV256            (0x0EU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV512            (0x0FU << RCC_CFGR_HPRE_Pos)

#define RCC_CFGR_PPRE1_DIV1             (0x00U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1_DIV2             (0x04U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1_DIV4             (0x05U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1_DIV8             (0x06U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1_DIV16            (0x07U << RCC_CFGR_PPRE1_Pos)

#define RCC_CFGR_SW_HSI_DIV6            (0x00U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSE                 (0x01U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSI                 (0x02U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_LSI                 (0x03U << RCC_CFGR_SW_Pos)

#define RCC_CFGR_SWS_HSI_DIV6           (0x00U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_HSE                (0x01U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_HSI                (0x02U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_LSI                (0x03U << RCC_CFGR_SWS_Pos)

/**
  * @brief  Configures the System clock frequency, HCLK, and PCLK1 prescalers.
  * @param  None
  * @retval None
  */
static void SetSysClockToDefine(void)
{
    __IO uint32_t StartUpCounter = 0, ClkSrcStatus = 1;
    uint32_t temp = 0;

#ifdef SYSCLK_HSE_XXMHz
     /* Enable HSE */
    RCC->CR |= (0x01U << RCC_CR_HSEON_Pos);

     /* Wait till HSE is ready and if Time out is reached exit */
    do
    {
        ClkSrcStatus = RCC->CR & RCC_CR_HSERDY_Msk;
        StartUpCounter++;
    }
    while((ClkSrcStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if ((RCC->CR & RCC_CR_HSERDY_Msk) != RESET)
    {
        ClkSrcStatus = (uint32_t)0x01;
    }
    else
    {
        ClkSrcStatus = (uint32_t)0x00;
    }

    SystemCoreClock = SYSCLK_HSE_XXMHz;

    if (ClkSrcStatus == (uint32_t)0x01)
    {
         /* Enable Prefetch Buffer */
        FLASH->ACR |= FLASH_ACR_PRFTBE;
         /* Flash 0 wait state, bit0~2 */
        FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;

         /* HCLK = SYSCLK */
        temp      = RCC->CFGR;
        temp     &= ~RCC_CFGR_HPRE_Msk;
        temp     |= RCC_CFGR_HPRE_DIV1;
        RCC->CFGR = temp;

         /* PCLK1 = HCLK */
        temp      = RCC->CFGR;
        temp     &= ~RCC_CFGR_PPRE1_Msk;
        temp     |= RCC_CFGR_PPRE1_DIV1;
        RCC->CFGR = temp;

         /* Select HSI as system clock source */
        RCC->CFGR &= ~RCC_CFGR_SW_Msk;
        RCC->CFGR |= RCC_CFGR_SW_HSE;

         /* Wait till HSE is used as system clock source */
        while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSE)
        {
            __ASM("nop");              /* __NOP(); */
        }
    }
    else
    {
        /* If HSE not ready within the given time, the program will stop here.
           User can add here some code to deal with this error */
        while (1)
        {
            /* please check Whether the crystal oscillator starts*/
        }
    }
#elif defined SYSCLK_HSI_XXMHz
    RCC->CR |= (0x01U << RCC_CR_HSION_Pos);

    while (!(RCC->CR & RCC_CR_HSIRDY_Msk))
    {
    }

    SystemCoreClock = SYSCLK_HSI_XXMHz;

     /* RCC->CFGR = RCC_CFGR_PPRE1_2; */
    FLASH->ACR = FLASH_ACR_LATENCY_0 | FLASH_ACR_PRFTBE;

     /* HCLK = SYSCLK */
    temp      = RCC->CFGR;
    temp     &= ~RCC_CFGR_HPRE_Msk;
    temp     |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR = temp;

     /* PCLK1 = HCLK */
    temp      = RCC->CFGR;
    temp     &= ~RCC_CFGR_PPRE1_Msk;
    temp     |= RCC_CFGR_PPRE1_DIV1;
    RCC->CFGR = temp;

     /* Select HSI as system clock source */
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    RCC->CFGR |= RCC_CFGR_SW_HSI;

     /* Wait till HSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSI)
    {
        __ASM("nop");                  /* __NOP(); */
    }
#endif
}

/**
  * @brief  Setup the microcontroller system Initialize the Embedded Flash Interface,
  *         SystemCoreClock variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
     /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
     /* Set HSION bit */
    RCC->CR |= (0x01U << RCC_CR_HSION_Pos);

     /* Reset SW, HPRE, PPRE1, PPRE2 and MCO bits */
    RCC->CFGR &= ~(RCC_CFGR_SW_Msk | RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_MCO_Msk);

     /* Reset HSEON */
    RCC->CR &= ~RCC_CR_HSEON_Msk;

     /* Disable all interrupts and clear pending bits */
    RCC->CIR = 0xFFFFFFFF;
    RCC->CIR = 0;

     /* Configure the System clock frequency, HCLK, PCLK1 prescalers */
     /* Configure the Flash Latency cycles and enable prefetch buffer */
    SetSysClockToDefine();
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

