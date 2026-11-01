/* Shared by BatTester and LoopSource: SYSCLK from MSI + PLL, with MSI locked
   to the 32.768 kHz X2 crystal. Moved out of BatTester's main.cpp unchanged
   (2026-11-01) so that both images run on the same clock. */
#include "main.h"

static bool g_lse_locked;

void SystemClock_Config()
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /* Start the 32.768 kHz crystal (X2) before the PLL, because MSIPLLEN can
     only be set once LSE is ready -- the hardware refuses otherwise
     (stm32l4xx_ll_rcc.h:2514). Polled directly rather than through
     HAL_RCC_OscConfig, which would spend LSE_STARTUP_TIMEOUT (5 s) here on
     every boot if the crystal turned out to be absent. One second is ample
     for a crystal that is fitted, and a missing one costs one second once. */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);
  {
    const uint32_t deadline = HAL_GetTick() + 1000u;
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0u) {
      if (static_cast<int32_t>(HAL_GetTick() - deadline) >= 0) {
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);
        break;
      }
    }
  }

  /* MSI + PLL → 80 MHz. */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }

  /* Lock MSI to the crystal. MSI is the PLL source, so this puts LSE accuracy
     on SYSCLK, on TIM6, and therefore on the 1 MSps DAC rate -- which is the
     number every reconstruction result is measured against (PLAN section 9).

     Measured on this board 2026-08-29, timing a 12.098 s scenario against the
     host clock: free-running MSI was -6329 ppm (0.63 % fast). That is the same
     order as the 0.65 % that DETECTOR_NOTES.md section 2 could not account
     for. Do not enable this without re-measuring; the point is the number. */
  if ((RCC->BDCR & RCC_BDCR_LSERDY) != 0u) {
    HAL_RCCEx_EnableMSIPLLMode();
    g_lse_locked = true;
  }
}

bool clock_locked_to_lse(void)
{
  return g_lse_locked;
}
