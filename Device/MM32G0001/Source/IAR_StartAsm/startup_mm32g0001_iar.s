;/***********************************************************************************************************************
;    @file     startup_mm32g0001_iar.s
;    @author   VV TEAM
;    @brief    THIS FILE PROVIDES ALL THE Device Startup File of MM32 Cortex-M
;              Core Device for ARM KEIL toolchain.
;  **********************************************************************************************************************
;    @attention
;
;    <h2><center>&copy; Copyright(c) <2023> <MindMotion></center></h2>
;
;      Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
;    following conditions are met:
;    1. Redistributions of source code must retain the above copyright notice,
;       this list of conditions and the following disclaimer.
;    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
;       the following disclaimer in the documentation and/or other materials provided with the distribution.
;    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
;       promote products derived from this software without specific prior written permission.
;
;      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
;    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
;    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
;    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;  *********************************************************************************************************************/

                MODULE  ?cstartup

                SECTION CSTACK:DATA:NOROOT(3)

                SECTION .intvec:CODE:NOROOT(2)

                EXTERN  __iar_program_start
                EXTERN  SystemInit
                PUBLIC  __vector_table
                PUBLIC  __Vectors
                PUBLIC  __Vectors_End
                PUBLIC  __Vectors_Size

                DATA
__vector_table
                DCD     sfe(CSTACK)                                             ;       Top of Stack
                DCD     Reset_Handler                                           ;       Reset Handler
                DCD     NMI_Handler                                             ; -14   NMI Handler
                DCD     HardFault_Handler                                       ; -13   Hard Fault Handler
                DCD     0                                                       ; -12   Reserved
                DCD     0                                                       ; -11   Reserved
                DCD     0                                                       ; -10   Reserved
                DCD     0                                                       ;  -9   Reserved
                DCD     0                                                       ;  -8   Reserved
                DCD     0                                                       ;  -7   Reserved
                DCD     0                                                       ;  -6   Reserved
                DCD     SVC_Handler                                             ;  -5   SVCall Handler
                DCD     0                                                       ;  -4   Reserved
                DCD     0                                                       ;  -3   Reserved
                DCD     PendSV_Handler                                          ;  -2   PendSV Handler
                DCD     SysTick_Handler                                         ;  -1   SysTick Handler

                ; External Interrupts
                DCD     IWDG_IRQHandler                                         ;   0   Independent WatchDog
                DCD     PVD_IRQHandler                                          ;   1   PVD through EXTI Line detect
                DCD     0                                                       ;   2   Reserved
                DCD     FLASH_IRQHandler                                        ;   3   FLASH
                DCD     RCC_IRQHandler                                          ;   4   RCC
                DCD     EXTI0_1_IRQHandler                                      ;   5   EXTI Line 0 and 1
                DCD     EXTI2_3_IRQHandler                                      ;   6   EXTI Line 2 and 3
                DCD     EXTI4_15_IRQHandler                                     ;   7   EXTI Line 4 to 15
                DCD     0                                                       ;   8   Reserved
                DCD     0                                                       ;   9   Reserved
                DCD     0                                                       ;  10   Reserved
                DCD     0                                                       ;  11   Reserved
                DCD     ADC_IRQHandler                                          ;  12   ADC
                DCD     TIM1_BRK_UP_TRG_COM_IRQHandler                          ;  13   TIM1 Break, Update, Trigger and Commutation
                DCD     TIM1_CC_IRQHandler                                      ;  14   TIM1 Capture Compare
                DCD     0                                                       ;  15   Reserved
                DCD     TIM3_IRQHandler                                         ;  16   TIM3
                DCD     0                                                       ;  17   Reserved
                DCD     0                                                       ;  18   Reserved
                DCD     TIM14_IRQHandler                                        ;  19   TIM14
                DCD     0                                                       ;  20   Reserved
                DCD     0                                                       ;  21   Reserved
                DCD     0                                                       ;  22   Reserved
                DCD     I2C1_IRQHandler                                         ;  23   I2C1
                DCD     0                                                       ;  24   Reserved
                DCD     SPI1_IRQHandler                                         ;  25   SPI1
                DCD     0                                                       ;  26   Reserved
                DCD     USART1_IRQHandler                                       ;  27   USART1
                DCD     USART2_IRQHandler                                       ;  28   USART2
                DCD     0                                                       ;  29   Reserved
                DCD     0                                                       ;  30   Reserved
                DCD     0                                                       ;  31   Reserved

__Vectors_End

__Vectors       EQU     __vector_table
__Vectors_Size  EQU     __Vectors_End - __Vectors


                THUMB

; Reset Handler

                PUBWEAK Reset_Handler
                SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__iar_program_start
                BX      R0


                PUBWEAK NMI_Handler
                PUBWEAK HardFault_Handler
                PUBWEAK SVC_Handler
                PUBWEAK PendSV_Handler
                PUBWEAK SysTick_Handler
                PUBWEAK IWDG_IRQHandler
                PUBWEAK PVD_IRQHandler

                PUBWEAK FLASH_IRQHandler
                PUBWEAK RCC_IRQHandler
                PUBWEAK EXTI0_1_IRQHandler
                PUBWEAK EXTI2_3_IRQHandler
                PUBWEAK EXTI4_15_IRQHandler
                PUBWEAK ADC_IRQHandler
                PUBWEAK TIM1_BRK_UP_TRG_COM_IRQHandler
                PUBWEAK TIM1_CC_IRQHandler
                PUBWEAK TIM3_IRQHandler
                PUBWEAK TIM14_IRQHandler
                PUBWEAK I2C1_IRQHandler
                PUBWEAK SPI1_IRQHandler
                PUBWEAK USART1_IRQHandler
                PUBWEAK USART2_IRQHandler

                SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler
HardFault_Handler
SVC_Handler
PendSV_Handler
SysTick_Handler
IWDG_IRQHandler
PVD_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_1_IRQHandler
EXTI2_3_IRQHandler
EXTI4_15_IRQHandler
ADC_IRQHandler
TIM1_BRK_UP_TRG_COM_IRQHandler
TIM1_CC_IRQHandler
TIM3_IRQHandler
TIM14_IRQHandler
I2C1_IRQHandler
SPI1_IRQHandler
USART1_IRQHandler
USART2_IRQHandler

Default_Handler
                B       .
                END
