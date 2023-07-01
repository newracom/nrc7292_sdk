 /**************************************************************************//**
 * @file     system_nrc7292.h
 * @brief    CMSIS Device System Header File for
 *           NRC7292 Device
 * @version  V1.0.0
 * @date     06. July 2021
 ******************************************************************************/
/*
 * Copyright (c) 2022 Newracom Inc. All rights reserved.
 */

#ifndef __SYSTEM_NRC7292_H__
#define __SYSTEM_NRC7292_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
  \brief Exception / Interrupt Handler Function Prototype
*/
typedef void(*VECTOR_TABLE_Type)(void);

/**
  \brief System Clock Frequency (Core Clock)
*/
extern uint32_t SystemCoreClock;

/**
  \brief Setup the microcontroller system.

   Initialize the System and update the SystemCoreClock variable.
 */
extern void SystemInit (void);


/**
  \brief  Update SystemCoreClock variable.

   Updates the SystemCoreClock with current core Clock retrieved from cpu registers.
 */
extern void SystemCoreClockUpdate (void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_NRC7292_H__ */
