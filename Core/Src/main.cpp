/* USER CODE BEGIN Header */
/*
  BUILD RELEASE. A Debug image does not work on this board.

    export PATH=~/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/bin:$PATH
    cmake --preset Release && cmake --build build/Release

  The toolchain is not on PATH by default, and CMakeLists.txt defaults
  CMAKE_BUILD_TYPE to Debug. Both traps lead to the same place.

  Why Release is mandatory, not a preference (measured on the scope
  2026-08-28). pack_half() must produce 1 Msample/s. At -O0 it produces
  about 0.8 Msample/s, so the DMA replays stale ring content:

    - scn tone : a constant +2.355 rad phase step every 5.07 ms. Only 57 %
                 of the energy stays in the carrier. Sidebands at +-200 Hz
                 sit 3.6 dB below it.
    - scn ref  : the call envelope repeats as a sawtooth every 4.8 ms, the
                 calls come out 4 to 5 times too long and merge, and the
                 run lasts 4.87 s instead of 1.245 s.

  None of that looks like a fault on a slow timebase. It looks like noise.

  Why Release is -O2 and not -Os. Code is about 19 KB of a 490 KB image;
  the rest is the const wave table, which no -O level moves. -Os saves
  2 KB, which is 0.2 % of the flash, and costs speed in the one loop that
  has no speed to give. If the fill is ever marginal again, add -flto:
  scenario::sample() is still one cross-module call for every sample.

  Two ST-LINKs are usually plugged in. Pin the L476 by serial, or the
  programmer grabs the U575 first.

    NUCLEO-L476RG  SN 0669FF313937424757082637
    NUCLEO-U575ZI  SN 004B00213235510C37333439   leave alone

  PROG=~/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI
  $PROG -l
  $PROG -c port=SWD freq=4000 mode=UR sn=0669FF313937424757082637 -w build/Release/BatTester.elf -v -rst

  Cutecom 115200 8N1 on that L476 VCP (not ACM0). DTR resets the MCU.

  Do not print on the CLI while a scenario runs. write_raw() blocks in
  HAL_UART_Transmit. A half buffer is 2.048 ms, which is about 23
  characters at 115200. The reply to 'help' is 300, and it starves the
  fill for 26 ms.
*/
/* USER CODE END Header */
#include "main.h"

#include "cli.hpp"
#include "scenario.hpp"
#include "synth.hpp"

extern "C" {
DAC_HandleTypeDef hdac1;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_dac1;
}

void SystemClock_Config();
static void MX_GPIO_Init();
static void MX_DMA_Init();
static void MX_DAC1_Init();
static void MX_TIM6_Init();
static void MX_USART2_Init();
static void autostart_poll();
static void run_led_poll();

int main()
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_TIM6_Init();
  MX_USART2_Init();
  /* DAC MSP must not take PA5: LD2 stays a GPIO. */
  {
    GPIO_InitTypeDef led = {};
    led.Pin = RUN_LED_Pin;
    led.Mode = GPIO_MODE_OUTPUT_PP;
    led.Pull = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RUN_LED_GPIO_Port, &led);
  }

  synth::init();
  scenario::init();
  cli::init();

  for (;;) {
    autostart_poll();
    synth::poll();
    cli::poll();
    run_led_poll();
  }
}

void SystemClock_Config()
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /* MSI + PLL → 80 MHz. LSE is not probed at boot: HAL waits
     LSE_STARTUP_TIMEOUT (5 s) if the 32 kHz crystal is missing. */
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
}

static void MX_DAC1_Init()
{
  DAC_ChannelConfTypeDef sConfig = {};

  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK) {
    Error_Handler();
  }

  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_TIM6_Init()
{
  TIM_MasterConfigTypeDef sMasterConfig = {};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 79;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_USART2_Init()
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_DMA_Init()
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

static void MX_GPIO_Init()
{
  GPIO_InitTypeDef GPIO_InitStruct = {};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(SYNC_GPIO_Port, SYNC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = SYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SYNC_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RUN_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RUN_LED_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUTOSTART_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(AUTOSTART_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USER_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(USER_BTN_GPIO_Port, &GPIO_InitStruct);
}

static bool autostart_held()
{
  return (HAL_GPIO_ReadPin(AUTOSTART_GPIO_Port, AUTOSTART_Pin) == GPIO_PIN_RESET)
      || (HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin) == GPIO_PIN_RESET);
}

static void autostart_poll()
{
  static uint32_t low_since;

  if (!autostart_held()) {
    low_since = 0;
    return;
  }
  if (synth::running()) {
    return;
  }
  if (low_since == 0u) {
    low_since = HAL_GetTick();
    return;
  }
  if ((HAL_GetTick() - low_since) < 20u) {
    return;
  }
  if (scenario::needs_table() && !scenario::table_ready()) {
    return;
  }
  synth::start();
  cli::say("autostart");
}

static void run_led_poll()
{
  if (!synth::running()) {
    HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET);
    return;
  }
  const uint32_t phase = HAL_GetTick() / 200u;
  HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin,
                    (phase & 1u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

extern "C" void Error_Handler()
{
  __disable_irq();
  while (true) {
  }
}

#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
