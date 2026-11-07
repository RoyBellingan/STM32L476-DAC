/*
  LoopSource: the second image of this tree. A 50 Hz sine on PA4 (DAC1 OUT1),
  read back by ADC1 on the same timer, measured on the board.
  Plan: digitalSpine todo/fluxStationLoopSource.md.

  BUILD RELEASE (the BatTester note in Core/Src/main.cpp applies):

    export PATH=~/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/bin:$PATH
    cmake --preset Release && cmake --build build/Release --target LoopSource -j8

  Without --target, the build makes BatTester too. -j8 is the limit on 6pack.
  Output: build/Release/LoopSource.elf and build/Release/LoopSource.map.

  Build of 2026-11-07 (b2426b4), no warnings in Loop/:
    FLASH  62 904 B of 1 MB   (6 %)
    RAM    72 224 B of 96 KB  (73 %), the budget that runs out first:
           adc_in g_acc 20 KB, g_raw 10 KB, g_ring 10 KB;
           sine_out g_ring, g_wave, g_basis 8 KB each; preview scratch 4 KB.
    RAM2   0 B of 32 KB. The linker script declares SRAM2 but puts no
           section in it, so a buffer only goes there with a new section.

  `id` prints rev=LOOP_GIT_REV, which CMake reads with git at configure
  time, not at build time. After a commit, run `cmake --preset Release`
  again, or the image reports the old rev. The rev does not show
  uncommitted changes either.

  Two ST-LINKs are usually plugged in. Pin the L476 by serial, or the
  programmer grabs the U575 first.

    NUCLEO-L476RG  SN 0669FF313937424757082637
    NUCLEO-U575ZI  SN 004B00213235510C37333439   leave alone

  PROG=~/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI
  $PROG -l
  $PROG -c port=SWD freq=4000 mode=UR sn=0669FF313937424757082637 -w build/Release/LoopSource.elf -v -rst

  ST-LINK VCP, 460800 8N1 (loopcfg::kUartBaud).

  Pins (Nucleo-L476RG). PA0 and PB0 stay free for the BatTester wiring.
    PA4 = A2   DAC1 OUT1, the sine
    PA1 = A1   ADC12_IN6, single-ended   -> channel "a"
    PA7 = D11  OPAMP2 VINM and ADC12_IN12  -> channel "m"
    PA6 = D12  OPAMP2 VINP and ADC12_IN11  -> channel "i"
    PB0 = A3   OPAMP2 VOUT and ADC12_IN15  -> channel "p"; keep A3 free
    PA5 = LD2  on while the amplitude is not 0
    PA9 = D8   scope trigger: high during each `read` capture
*/
#include "main.h"

#include "adc_in.hpp"
#include "cli.hpp"
#include "loop_cfg.hpp"
#include "pga.hpp"
#include "sine_out.hpp"

extern "C" {
DAC_HandleTypeDef hdac1;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_dac1;
}

void SystemClock_Config();

namespace {

void gpio_init()
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET);
	GPIO_InitTypeDef g = {};
	g.Pin = RUN_LED_Pin;
	g.Mode = GPIO_MODE_OUTPUT_PP;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(RUN_LED_GPIO_Port, &g);
}

void dma_init()
{
	__HAL_RCC_DMA1_CLK_ENABLE();
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

void dac_init()
{
	hdac1.Instance = DAC1;
	if (HAL_DAC_Init(&hdac1) != HAL_OK) {
		Error_Handler();
	}
	DAC_ChannelConfTypeDef c = {};
	c.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
	c.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
	c.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	c.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
	c.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
	if (HAL_DAC_ConfigChannel(&hdac1, &c, DAC_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}
}

void tim6_init()
{
	htim6.Instance = TIM6;
	htim6.Init.Prescaler = 0;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.Period = loopcfg::kTimDiv - 1u;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
		Error_Handler();
	}
	TIM_MasterConfigTypeDef m = {};
	m.MasterOutputTrigger = TIM_TRGO_UPDATE;
	m.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &m) != HAL_OK) {
		Error_Handler();
	}
}

void usart2_init()
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = loopcfg::kUartBaud;
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
	/* Above the DMA interrupts. The L476 USART holds one received byte, which
	   is 87 us at 115200 and 22 us at 460800; the DAC fill and the ADC block each run ~0.2 ms in
	   their DMA interrupts. At the shared MSP's priority 5, ~3 % of the bytes
	   were lost (measured 2026-11-01). The UART handler is a few us, and the
	   DMA interrupts have 10 ms of margin. */
	HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
}

}

int main()
{
	HAL_Init();
	SystemClock_Config();
	gpio_init();
	dma_init();
	dac_init();
	tim6_init();
	usart2_init();

	sine_out::init();
	pga::init();
	adc_in::init();
	if (!sine_out::start()) {
		Error_Handler();
	}
	/* One timer starts both: DAC sample j and ADC scan j share trigger j. */
	SET_BIT(TIM6->CR1, TIM_CR1_CEN);

	cli::init();
	for (;;) {
		cli::poll();
		HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin,
		                  (sine_out::amp_codes() > 0.0f) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
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
