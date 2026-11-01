#include "pga.hpp"

#include "main.h"

namespace pga {
namespace {

uint32_t g_gain;
bool g_ext;
float g_nominal;

}

void init()
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	/* PA6 = OPAMP2_VINP and ADC12_IN11; PB0 = OPAMP2_VOUT and ADC12_IN15.
	   The ADC reads both, so both need the ADC switch (ASCR). */
	GPIO_InitTypeDef g = {};
	g.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
	g.Pull = GPIO_NOPULL;
	g.Pin = GPIO_PIN_6 | GPIO_PIN_7;   /* PA7 = OPAMP2_VINM and ADC12_IN12 */
	HAL_GPIO_Init(GPIOA, &g);
	g.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOB, &g);

	__HAL_RCC_OPAMP_CLK_ENABLE();
	OPAMP2->CSR = 0;
	OPAMP1->CSR = 0;
	/* VDDA > 2.4 V. Shared bit, in OPAMP1's register; write it with both
	   op-amps off. */
	OPAMP1->CSR |= OPAMP1_CSR_OPARANGE;
	g_gain = 0;
}

bool set_gain(uint32_t gain, bool ext_vinm)
{
	/* VMSEL 10: VINM not connected. 00: VINM on PA7 (in PGA mode, "for
	   filtering"; which node of the network it reaches is what we test). */
	uint32_t csr = ext_vinm ? 0u : OPAMP_CSR_VMSEL_1;
	switch (gain) {
	case 0:
		OPAMP2->CSR = 0;
		g_gain = 0;
		g_ext = false;
		g_nominal = 0.0f;
		return true;
	case 1:
		csr |= OPAMP_CSR_OPAMODE_1 | OPAMP_CSR_OPAMODE_0;   /* follower */
		break;
	case 2:
		csr |= OPAMP_CSR_OPAMODE_1;
		break;
	case 4:
		csr |= OPAMP_CSR_OPAMODE_1 | OPAMP_CSR_PGGAIN_0;
		break;
	case 8:
		csr |= OPAMP_CSR_OPAMODE_1 | OPAMP_CSR_PGGAIN_1;
		break;
	case 16:
		csr |= OPAMP_CSR_OPAMODE_1 | OPAMP_CSR_PGGAIN_1 | OPAMP_CSR_PGGAIN_0;
		break;
	default:
		return false;
	}
	/* VPSEL = 0: VINP on PA6. Normal power (OPALPM = 0), factory trim. */
	OPAMP2->CSR = csr;
	OPAMP2->CSR = csr | OPAMP2_CSR_OPAEN;
	g_gain = gain;
	g_ext = ext_vinm;
	g_nominal = static_cast<float>(gain);
	return true;
}

void set_standalone(float nominal)
{
	/* OPAMODE 00: plain op-amp, internal resistors out. VINP = PA6, VINM = PA7
	   (VMSEL 00), VOUT = PB0. The gain is set by the external R1, R2. */
	OPAMP2->CSR = 0;
	OPAMP2->CSR = OPAMP2_CSR_OPAEN;
	g_gain = kStandalone;
	g_ext = true;
	g_nominal = nominal;
}

float nominal()
{
	return g_nominal;
}

uint32_t gain()
{
	return g_gain;
}

bool ext_vinm()
{
	return g_ext;
}

}
