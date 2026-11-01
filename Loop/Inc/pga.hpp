#ifndef PGA_HPP
#define PGA_HPP

#include <cstdint>

/* OPAMP2 as the input PGA: VINP = PA6 (D12), VOUT = PB0 (A3), VINM not used.

   DS10198 Table 86: gains 2, 4, 8, 16, gain error +-1 % max, offset +-1.5 mV
   at 25 degC, GBW 1.6 MHz typ, output VSSA+0.1 .. VDDA-0.1 V. The internal R1
   returns to GND (DS10198 note 5; stm32l4xx_ll_opamp.h:206, VINM is only for
   filtering), so the DC level is multiplied too: for a centred output the
   input must sit at VDDA / (2 G). Do not use OPAMP1: its output is PA3, the
   ST-LINK VCP RX. Keep A3 (PB0) free of other loads: <= 450 uA in PGA mode. */
namespace pga {

void init();                 /* clock on, OPA_RANGE high, OPAMP2 off */
/* 0 = off, 1 = follower, 2 / 4 / 8 / 16 = PGA. ext_vinm: connect VINM to PA7
   (D11); with gain 0 or 1 it changes nothing useful. */
bool set_gain(uint32_t g, bool ext_vinm);
uint32_t gain();             /* 0 off, 1..16 internal, kStandalone */
bool ext_vinm();

/* Standalone op-amp with external feedback: R2 from PB0 (A3) to PA7 (D11), R1
   from PA7 to a capacitor to GND (DC gain 1, so the output sits on the input
   bias) or to a reference. nominal = 1 + R2/R1, for the report only: the gain
   is measured, p over i. */
inline constexpr uint32_t kStandalone = 100u;
void  set_standalone(float nominal);
float nominal();

}

#endif
