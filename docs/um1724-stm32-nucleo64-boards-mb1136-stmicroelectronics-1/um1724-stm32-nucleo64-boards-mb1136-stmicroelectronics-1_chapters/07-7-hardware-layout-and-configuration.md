## 7 Hardware layout and configuration

The STM32 Nucleo board is designed around the STM32 microcontrollers in a 64-pin LQFP package.

Figure 2 shows the connections between the STM32 and its peripherals (ST-LINK/V2-1, push-button, LED, ARDUINO ®  connectors, and ST morpho connector).

Figure 3 and Figure 4 show the location of these features on the STM32 Nucleo board. Figure 5 shows the mechanical dimensions of the STM32 Nucleo board.

### 7.1 Board diagram and layout

Figure 2. Hardware block diagram

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_fe82c9c02eabc6793cf57b8bba840e0a9452cf5638b10db10ed12da37340e16b.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_8bb08c99fc7108da3e64242368569bb26ea8330cd596ac8aa0e6613840e3eefc.png)

90

Figure 3. Top layout

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_f03909e697c164e9e8900e44c4cafde190a77d8eb7760fa5ddb32d725b255bdd.png)

1. Crystal might be present or not depending on the board version. Refer to Section 7.9.2 .

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_8f5d5391e81d4c2d2e4414ea87aa382e9b12a9063d528385d22e25ef12976af3.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_0d6ece4667210e157d4abe0cd6d8c4d19c7a17e6951619fbbe9bba64eecbbead.png)

Figure 4. Bottom layout

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_0d4083cb20c9287f9c4a2f981954848c23593d3ce248b55ba3b48263035706d2.png)

90

### 7.2 Mechanical dimensions

Figure 5. STM32 Nucleo board mechanical dimensions

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_e2fbcb2ba43b68a00a2bd86b19f9b0266ba26254965165884274275dcc77b000.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

### 7.3 Pre-cut PCB

The STM32 Nucleo board is divided into two parts: the ST-LINK part and the target STM32 part. The ST-LINK part of the PCB can be cut out to reduce the board size. In this case, the remaining target STM32 part can only be powered by VIN, E5V, and +3.3V on the ST morpho connector CN7, or VIN and +3.3V on ARDUINO ®  connector CN6. It is still possible to use the ST-LINK part to program the main STM32 using wires between CN4 and SWD signals available on the ST morpho connector (SWCLK CN7 pin 15 and SWDIO CN7 pin 13).

### 7.4 Embedded ST-LINK/V2-1

The ST-LINK/V2-1 programming and debugging tool is integrated into the STM32 Nucleo board.

The embedded ST-LINK/V2-1 supports only SWD for STM32 devices. For information about debugging and programming features, refer to the user manual ST-LINK/V2 in-circuit debugger/programmer for STM8 and STM32 (UM1075), which describes in detail all the STLINK/V2 features.

The changes versus the ST-LINK/V2 version are listed below.

- New features supported on ST-LINK/V2-1:
- -USB software re-enumeration
- -Virtual COM port interface on USB
- -Mass storage interface on USB
- -USB power management request for more than 100 mA power on USB
- Features not supported on ST-LINK/V2-1:
- -SWIM interface
- -Minimum supported application voltage limited to 3 V

There are two different ways to use the embedded ST-LINK/V2-1 depending on the jumper states (see Table 4 and Figure 7.4.1 ):

- Program/debug the on-board STM32 ( Section 7.4.2 )
- Program/debug an MCU in an external application board using a cable connected to SWD connector CN4 ( Section 7.4.5 ).

Table 4. Jumper states

| Jumper state         | Description                                                               |
|----------------------|---------------------------------------------------------------------------|
| Both CN2 jumpers ON  | ST-LINK/V2-1 functions enabled for on-board programming (default)         |
| Both CN2 jumpers OFF | ST-LINK/V2-1 functions enabled for external CN4 connector (SWD supported) |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bb08c99fc7108da3e64242368569bb26ea8330cd596ac8aa0e6613840e3eefc.png)

90

#### 7.4.1 Driver

Before connecting the Nucleo-64 board to a Windows ®  PC via USB, a driver for STLINK/V2-1 must be installed. It can be downloaded from the www.st.com website.

In case the STM32 Nucleo-64 board is connected to the PC before installing the driver, the PC device manager might report some Nucleo interfaces as 'Unknown'.

To recover from this situation, after installing the dedicated driver, the association of 'Unknown' USB devices found on the STM32 Nucleo-64 board to this dedicated driver, must be updated in the device manager manually.

Note: It is recommended to proceed using a USB Composite Device, as shown in Figure 6.

Figure 6. Updating the list of drivers in the Device Manager

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_0576ecb9b503140d4d47d1529c7093a9992982eb98220161f1edd86893abdfc9.png)

#### 7.4.2 ST-LINK/V2-1 firmware upgrade

The ST-LINK/V2-1 embeds a firmware upgrade mechanism for the in-situ upgrade through the USB port. As the firmware might evolve during the lifetime of the ST-LINK/V2-1 product (for example new functionality, bug fixes, support for new microcontroller families), it is recommended to visit www.st.com website before starting to use the STM32 Nucleo board and periodically, to stay updated with the latest firmware version.

#### 7.4.3 Mass storage interface

The ST-LINK/V2-1 provides an optional mass storage interface, allowing the user to program a target application (binary or hex file) through a virtual mass storage disk. The availability of this interface can be selected during the firmware upgrade, as detailed in the technical note Overview of ST-LINK derivatives (TN1235). Make sure that the host PC system (including antivirus applications) allows the mass storage disk before activating the interface.

#### 7.4.4 Using the ST-LINK/V2-1 to program and debug the STM32 on board

To program the STM32 on the board, plug in the two jumpers on CN2, as shown in red in Figure 7 . Do not use the CN4 connector as this could disturb communication with the STM32 microcontroller of the STM32 Nucleo board.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

Figure 7. Connecting the STM32 Nucleo board to program the on-board STM32

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_fd2b5598cae07edef347cb090138b49d13337e2bcc6b806d48a069421da6640d.png)

#### 7.4.5 Using ST-LINK/V2-1 to program and debug an external STM32 application

It is very easy to use the ST-LINK/V2-1 to program the STM32 on an external application. Simply remove the two jumpers from CN2 as illustrated in Figure 8: Using ST-LINK/V2-1 to program the STM32 on an external application , and connect the application to the CN4 debug connector according to Table 5 .

Note: SB12 NRST (target STM32 RESET) must be OFF if CN4 pin 5 is used in the external application.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_fa67eb8f66eab24ad09d37221a5cc6eca8e8ec349edfc8b9f421018d8037962f.png)

90

Table 5. Debug connector CN4 (SWD)

|   Pin | CN4        | Designation           |
|-------|------------|-----------------------|
|     1 | VDD_TARGET | VDD from application  |
|     2 | SWCLK      | SWD clock             |
|     3 | GND        | ground                |
|     4 | SWDIO      | SWD data input/output |
|     5 | NRST       | RESET of target STM32 |
|     6 | SWO        | Reserved              |

Figure 8. Using ST-LINK/V2-1 to program the STM32 on an external application

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_d22e12a68dde89bbbd7e80d50c736329cda8c567d34a3d3667ec161e08533162.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

### 7.5 Power supply and power selection

The power supply is provided either by the host PC through the USB cable, or by an external source: VIN (From 7 V to 12 V), E5V (5 V), or +3.3V power supply pins on CN6 or CN7. In case VIN, E5V, or +3.3V is used to power the STM32 Nucleo board, using an external power supply unit or auxiliary equipment, this power source must comply with the standard EN-62368-1: 2014/A11:2017, and must be safety extralow voltage (SELV) with limited power capability.

#### 7.5.1 Power supply input from the USB connector

The ST-LINK/V2-1 supports USB power management allowing to request more than 100 mA current to the host PC.

All parts of the STM32 Nucleo board and shield can be powered from the ST-LINK USB connector CN1 (U5V or VBUS). Note that only the ST-LINK part is power supplied before the USB enumeration as the host PC only provides 100 mA to the board at that time. During the USB enumeration, the STM32 Nucleo board requires 300 mA of current to the host PC. If the host can provide the required power, the targeted STM32 microcontroller is powered and the red LED LD3 is turned ON, thus the STM32 Nucleo board and its shield can consume a maximum of 300 mA current, not more. If the host is not able to provide the required current, the targeted STM32 microcontroller and the MCU part including the extension board are not power supplied. As a consequence, the red LED LD3 remains turned OFF. In such a case it is mandatory to use an external power supply as explained in the next Section 7.5.2: External power supply inputs: VIN and E5V .

When the board is power supplied by USB (U5V) a jumper must be connected between pin 1 and pin 2 of JP5 as shown in Table 8 .

JP1 is configured according to the maximum current consumption of the board when powered by USB (U5V). JP1 jumper can be set in case the board is powered by USB and maximum current consumption on U5V does not exceed 100 mA (including an eventual extension board or ARDUINO ®  shield). In such a condition, USB enumeration always succeeds since no more than 100 mA is requested from the PC. Possible configurations of JP1 are summarized in Table 6 .

Table 6. JP1 configuration table

| Jumper state   | Power supply          | Allowed current   |
|----------------|-----------------------|-------------------|
| JP1 jumper OFF | USB power through CN1 | 300 mA max        |
| JP1 jumper ON  | USB power through CN1 | 100 mA max        |

##### Warning:

If the maximum current consumption of the NUCLEO and its extension boards exceeds 300 mA, it is mandatory to power the NUCLEO using an external power supply connected to E5V or VIN.

In case the board is powered by a USB charger, there is no USB enumeration, so the led LD3 remains set to OFF permanently and the target STM32 is not powered. In this specific case, the jumper JP1 needs to be set to ON, to allow target STM32 to be powered anyway.

Note:

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_04216e8fa57c8b9aa0e8f0a66f5b8df565eb96f4348fee201d8e5f91cd96a94c.png)

90

#### 7.5.2 External power supply inputs: VIN and E5V

The external power sources VIN and E5V are summarized in Table 7. When the board is power supplied by VIN or E5V, the jumper configuration must be the following:

- Jumper on JP5 pin 2 and pin 3
- Jumper removed on JP1

Table 7. External power sources

| Input power name   | Connectors pins      | Voltage range    | Max current   | Limitation                                                                                                                                                                                                |
|--------------------|----------------------|------------------|---------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| VIN                | CN6 pin 8 CN7 pin 24 | 7 V to 12 V      | 800 mA        | From 7 V to 12 V only and input current capability is linked to input voltage: 800 mA input current when Vin = 7 V 450 mA input current when 7 V < Vin <= 9 V 250 mA input current when 9 V < Vin <= 12 V |
| E5V                | CN7 pin 6            | 4.75 V to 5.25 V | 500 mA        | -                                                                                                                                                                                                         |

Table 8. Power-related jumper

| Jumper   | Description                                                                                   | Description                                                                                   |
|----------|-----------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| JP5      | U5V (ST-LINK VBUS) is used as a power source when JP5 is set as shown below (Default setting) | U5V (ST-LINK VBUS) is used as a power source when JP5 is set as shown below (Default setting) |
| JP5      |                                                                                               | 1 2 3 E5V U5V                                                                                 |
| JP5      | VIN or E5V is used as a power source when JP5 is set as shown below.                          | VIN or E5V is used as a power source when JP5 is set as shown below.                          |
| JP5      |                                                                                               | 1 2 3 E5V U5V                                                                                 |

##### Using VIN or E5V as an external power supply

VIN or E5V can be used as an external power supply in case the current consumption of the STM32 Nucleo and extensions boards exceeds the allowed current on USB. In this condition, it is still possible to use the USB for communication, programming, or debugging only, but it is mandatory to power supply the board first using VIN or E5V and then connect the USB cable to the PC. Proceeding this way ensures that the enumeration occurs thanks to the external power source.

The following power sequence procedure must be respected:

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_b7ee11efb455d2cdc7e03b0bccdaa97737dded4d8761df94855ebd1be7b939c8.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_65344fa6793413928ab8425ec5bb792c61d4e5062cddcdf2901e497c759efbd6.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_04216e8fa57c8b9aa0e8f0a66f5b8df565eb96f4348fee201d8e5f91cd96a94c.png)

1. Connect the jumper between pin 2 and pin 3 of JP5
2. Check that JP1 is removed
3. Connect the external power source to VIN or E5V
4. Power on the external power supply 7 V&lt; VIN &lt; 12 V to VIN, or 5 V for E5V
5. Check that LD3 is turned ON
6. Connect the PC to USB connector CN1

If this order is not respected, the board might be supplied by VBUS first then by VIN or E5V, and the following risks might be encountered:

1. If more than 300 mA current is needed by the board, the PC might be damaged or the current supply can be limited by the PC. As a consequence, the board is not powered correctly.
2. 300 mA is requested at enumeration (since JP1 must be OFF) so there is a risk that the request is rejected and the enumeration does not succeed if the PC cannot provide such current. Consequently, the board is not power supplied (LED LD3 remains OFF).

#### 7.5.3 External power supply input: +3.3V

It can be of interest to use the +3.3V (CN6 pin 4 or CN7 pin 12 and pin 16) directly as power input for instance in case the 3.3V is provided by an extension board. When the STM32 Nucleo board is power supplied by +3.3V, the ST-LINK is not powered, thus the programming and debug features are unavailable. The +3.3V external power source is summarized in Table 9.

Table 9. +3.3 V external power source

| Input power name   | Connectors pins                 | Voltage range   | Limitation                                                       |
|--------------------|---------------------------------|-----------------|------------------------------------------------------------------|
| +3.3V              | CN6 pin 4 CN7 pin 12 and pin 16 | 3 V to 3.6 V    | Used when the ST-LINK part of the PCB is cut or SB2 and SB12 OFF |

Two different configurations are possible when using +3.3V to power the board:

- ST-LINK is removed (PCB cut) or
- SB2 (3.3V regulator) and SB12 (NRST) are OFF.

#### 7.5.4 External power supply output

When powered by USB, VIN, or E5V, the +5V (CN6 pin 5 or CN7 pin 18) can be used as an output power supply for an ARDUINO ®  shield or an extension board. In this case, the maximum current of the power source specified in Table 7 must be respected.

The +3.3V (CN6 pin 4 or CN7 pin 12 and 16) can be used also as power supply output. The current is limited by the maximum current capability of the regulator U4 (500 mA max).

90

### 7.6 LEDs

The tricolor LED (green, orange, red) LD1 (COM) provides information about ST-LINK communication status. LD1 default color is red. LD1 turns to green to indicate that communication is in progress between the PC and the ST-LINK/V2-1, with the following setup:

- Slow blinking Red/Off: at power-on before USB initialization
- Fast blinking Red/Off: after the first correct communication between the PC and ST-LINK/V2-1 (enumeration)
- Red LED On: when the initialization between the PC and ST-LINK/V2-1 is complete
- Green LED On: after a successful target communication initialization
- Blinking Red/Green: during communication with the target
- Green On: communication finished and successful
- Orange On: Communication failure

User LD2 : the green LED is a user LED connected to ARDUINO ®  signal D13 corresponding to STM32 I/O PA5 (pin 21) or PB13 (pin 34) depending on the STM32 target. Refer to Table 11 to Table 23 when:

- the I/O is HIGH value, the LED is on
- the I/O is LOW, the LED is off

LD3 PWR : the red LED indicates that the STM32 part is powered and +5V power is available.

### 7.7 Push-buttons

B1 USER : the user button is connected to the I/O PC13 (pin 2) of the STM32 microcontroller.

B2 RESET : this push-button is connected to NRST, and is used to RESET the STM32 microcontroller.

Note: The blue and black plastic hats that are placed on the push-buttons can be removed if necessary, for example, when a shield or an application board is plugged into the top of the Nucleo board. This will avoid pressure on the buttons and consequently a possible permanent target STM32 RESET.

### 7.8 JP6 (IDD)

Jumper JP6, labeled IDD, is used to measure the STM32 microcontroller consumption by removing the jumper and connecting an ammeter:

- Jumper ON: STM32 microcontroller is powered (default).
- Jumper OFF: an ammeter must be connected to measure the STM32 microcontroller current. If there is no ammeter, the STM32 microcontroller is not powered.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

### 7.9 OSC clock

#### 7.9.1 OSC clock supply

There are four ways to configure the pins corresponding to the external high-speed clock (HSE):

- MCO from ST-LINK : MCO output of ST-LINK MCU is used as an input clock. This frequency cannot be changed, it is fixed at 8 MHz and connected to PF0/PD0/PH0-OSC_IN of the STM32 microcontroller.

The following configuration is needed:

- -SB55 OFF and SB54 ON
- -SB16 and SB50 ON
- -R35 and R37 removed
- HSE oscillator on-board from X3 crystal (not provided): for typical frequencies and its capacitors and resistors, refer to the STM32 microcontroller datasheet. Refer to the AN2867 Application note for oscillator design guide for STM32 microcontrollers.The X3 crystal has the following characteristics: 8 MHz, 16 pF, 20 ppm, and DIP footprint. It is recommended to use 9SL8000016AFXHF0 manufactured by Hong Kong X'tals Limited.

The following configuration is needed:

- -SB54 and SB55 OFF
- -R35 and R37 soldered
- -C33 and C34 soldered with 20 pF capacitors
- -SB16 and SB50 OFF
- Oscillator from external PF0/PD0/PH0 : from an external oscillator through pin 29 of the CN7 connector.

The following configuration is needed:

- -SB55 ON
- -SB50 OFF
- -R35 and R37 removed
- HSE not used : PF0/PD0/PH0 and PF1/PD1/PH1 are used as GPIOs instead of clocks

The following configuration is needed:

- -SB54 and SB55 ON
- -SB16 and SB50 (MCO) OFF
- -R35 and R37 removed

There are two possible default configurations of the HSE pins, depending on the version of the STM32 Nucleo board hardware.

The board version MB1136 C-01 or MB1136 C-02 is mentioned on the sticker, placed on the bottom side of the PCB.

The board marking MB1136 C-01 corresponds to a board, configured as HSE not used.

The board marking MB1136 C-02 (or higher) corresponds to a board, configured to use ST-LINK MCO as the clock input.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bb08c99fc7108da3e64242368569bb26ea8330cd596ac8aa0e6613840e3eefc.png)

90

Note:

For NUCLEO-L476RG and NUCLEO-L452RE the ST-LINK MCO output is not connected to OSCIN to reduce power consumption in low power mode. Consequently, NUCLEO-L476RG and NUCLEO-L452RE configurations correspond to HSE not used.

#### 7.9.2 OSC 32 KHz clock supply

There are three ways to configure the pins corresponding to the low-speed clock (LSE):

- On-board oscillator: X2 crystal. Refer to the Oscillator design guide for STM8S, STM8A and STM32 microcontrollers application note ( AN2867) for oscillator design guide for STM32 microcontrollers. It is recommended to use ABS25-32.768KHZ-6-T, manufactured by Abracon Corporation.
- Oscillator from external PC14 : from external oscillator through the pin 25 of CN7 connector.

The following configuration is needed:

- -SB48 and SB49 ON
- -R34 and R36 removed
- LSE not used : PC14 and PC15 are used as GPIOs instead of low-speed clocks.

The following configuration is needed:

- -SB48 and SB49 ON
- -R34 and R36 removed

There are three possible default configurations of the LSE depending on the version of the STM32 Nucleo board hardware.

The board version MB1136 C-01 or MB1136 C-02 is mentioned on the sticker placed on the bottom side of the PCB.

The board marking MB1136 C-01 corresponds to a board configured as LSE not used.

The board marking MB1136 C-02 (or higher) corresponds to a board configured with an onboard 32 KHz oscillator.

The board marking MB1136 C-03 (or higher) corresponds to a board using a new LSE crystal (ABS25) and C26, C31, and C32 value update.

### 7.10 USART communication

The USART2 interface available on PA2 and PA3 of the STM32 microcontroller can be connected to ST-LINK MCU, ST morpho connector, or ARDUINO ®  connector. The choice can be changed by setting the related solder bridges. By default, the USART2 communication between the target STM32 and ST-LINK MCU is enabled, to support virtual COM port (SB13 and SB14 ON, SB62 and SB63 OFF). If the communication between the target STM32 PA2 (D1) or PA3 (D0) and shield or extension board is required, SB62 and SB63 must be ON, while SB13 and SB14 must be OFF. In such a case, it is possible to connect another USART to the ST-LINK MCU using flying wires between the ST morpho connector and CN3. For instance, on NUCLEO-F103RB it is possible to use USART3 available on PC10 (TX) and PC11 (RX). Two flying wires need to be connected as follows:

- PC10 (USART3_TX) available on CN7 pin 1 to CN3 pin RX
- PC11 (USART3_RX) available on CN7 pin 2 to CN3 pin TX

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

### 7.11 Solder bridges

Table 10. Solder bridges

| Bridge                       | State (1)   | Description                                                                                                                                                           |
|------------------------------|-------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| SB54, SB55 (X3 crystal) (2)  | OFF         | X3, C33, C34, R35, and R37 provide a clock as shown in electrical schematics PF0/PD0/PH0, PF1/PD1/PH1 are disconnected from CN7.                                      |
| SB54, SB55 (X3 crystal) (2)  | ON          | PF0/PD0/PH0 and PF1/PD1/PH1 are connected to CN7. (R35, R37, and SB50 must not be fitted).                                                                            |
| SB3, SB5, SB7,SB9 (DEFAULT)  | ON          | Reserved, do not modify.                                                                                                                                              |
| SB4,SB6, SB8,SB10 (RESERVED) | OFF         | Reserved, do not modify.                                                                                                                                              |
| SB48, SB49 (X2 crystal) (3)  | OFF         | X2, C31, C32, R34, and R36 deliver a 32 KHz clock. PC14 and PC15 are not connected to CN7.                                                                            |
| SB48, SB49 (X2 crystal) (3)  | ON          | PC14 and PC15 are only connected to CN7. Remove only R34 and R36.                                                                                                     |
| SB17 (B1-USER)               | ON          | B1 push button is connected to PC13.                                                                                                                                  |
| SB17 (B1-USER)               | OFF         | B1 push button is not connected to PC13.                                                                                                                              |
| SB12 (NRST)                  | ON          | The NRST signal of the CN4 connector is connected to the NRST pin of the STM32.                                                                                       |
| SB12 (NRST)                  | OFF         | The NRST signal of the CN4 connector is not connected to the NRST pin of the STM32.                                                                                   |
| SB15 (SWO)                   | ON          | The SWO signal of the CN4 connector is connected to PB3.                                                                                                              |
| SB15 (SWO)                   | OFF         | The SWO signal is not connected.                                                                                                                                      |
| SB11 (STM_RST)               | OFF         | No incidence on STM32F103CBT6 (ST-LINK MCU) NRST signal.                                                                                                              |
| SB11 (STM_RST)               | ON          | STM32F103CBT6 (ST-LINK MCU) NRST signal is connected to GND.                                                                                                          |
| SB1 (USB-5V)                 | OFF         | USB power management is functional.                                                                                                                                   |
| SB1 (USB-5V)                 | ON          | USB power management is disabled.                                                                                                                                     |
| SB2 (3.3 V)                  | ON          | Output of voltage regulator LD39050PU33R is connected to 3.3V.                                                                                                        |
| SB2 (3.3 V)                  | OFF         | Output of voltage regulator LD39050PU33R is not connected.                                                                                                            |
| SB21 (LD2-LED)               | ON          | Green user LED LD2 is connected to D13 of ARDUINO ® signal.                                                                                                           |
| SB21 (LD2-LED)               | OFF         | Green user LED LD2 is not connected.                                                                                                                                  |
| SB56,SB51 (A4 and A5)        | ON          | PC1 and PC0 (ADC in) are connected to A4 and A5 (pin 5 and pin 6) on ARDUINO ® connector CN8 and ST morpho connector CN7. Thus SB46 and SB52 must be OFF.             |
| SB56,SB51 (A4 and A5)        | OFF         | PC1 and PC0 (ADC in) are disconnected to A4 and A5 (pin 5 and pin 6) on ARDUINO ® connector CN8 and ST morpho connector CN7.                                          |
| SB46,SB52 (I2C on A4 and A5) | OFF         | PB9 and PB8 (I2C) are disconnected to A4 and A5 (pin 5 and pin 6) on ARDUINO ® connector CN8 and ST morpho connector CN7.                                             |
| SB46,SB52 (I2C on A4 and A5) | ON          | PB9 and PB8 (I2C) are connected to A4 and A5 (pin 5 and pin 6) on ARDUINO ® connector CN8 and ST morpho connector CN7 as I2C signals. Thus SB56 and SB51 must be OFF. |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_04216e8fa57c8b9aa0e8f0a66f5b8df565eb96f4348fee201d8e5f91cd96a94c.png)

90

Table 10. Solder bridges (continued)

| Bridge                     | State (1)   | Description                                                                                                                                                                 |
|----------------------------|-------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| SB45 (VBAT/VLCD)           | ON          | VBAT or VLCD on STM32 is connected to VDD.                                                                                                                                  |
| SB45 (VBAT/VLCD)           | OFF         | VBAT or VLCD on STM32 is not connected to VDD.                                                                                                                              |
| SB57 (VDDA/VREF+)          | ON          | VDDA/VREF+ on STM32 is connected to VDD.                                                                                                                                    |
| SB57 (VDDA/VREF+)          | OFF         | VDDA/VREF+ on STM32 is not connected to VDD and can be provided from pin 8 of CN5 (Used for external VREF+ provided by ARDUINO ® shield)                                    |
| SB62, SB63 (USART)         | OFF         | PA2 and PA3 on STM32 are disconnected to D1 and D0 (pin 2 and pin 1) on ARDUINO ® connector CN9 and ST morpho connector CN10.                                               |
| SB62, SB63 (USART)         | ON          | PA2 and PA3 on STM32 are connected to D1 and D0 (pin 2 and pin 1) on ARDUINO ® connector CN9 and ST morpho connector CN10 as USART signals. Thus SB13 and SB14 must be OFF. |
| SB13, SB14 (ST-LINK-USART) | ON          | PA2 and PA3 on STM32F103CBT6 (ST-LINK MCU) are connected to PA3 and PA2 on STM32 to have USART communication between them. Thus SB61, SB62, and SB63 must be OFF.           |
| SB13, SB14 (ST-LINK-USART) | OFF         | PA2 and PA3 on STM32F103CBT6 (ST-LINK MCU) are disconnected to PA3 and PA2 on STM32.                                                                                        |
| SB16,SB50(MCO) (2)         | OFF         | MCO on STM32F103CBT6 (ST-LINK MCU) are disconnected to PF0/PD0/PH0 on STM32.                                                                                                |
| SB16,SB50(MCO) (2)         | ON          | MCO on STM32F103CBT6 (ST-LINK MCU) are connected to PF0/PD0/PH0 on STM32.                                                                                                   |

2. The default configuration depends on the board version. Refer to Section 7.9.1: OSC clock supply for details.
3. The default configuration depends on the board version. Refer to Section 7.9.2: OSC 32 KHz clock supply for details.

All the other solder bridges present on the STM32 Nucleo board are used to configure several I/Os and power supply pins for compatibility of features and pinout with STM32 supported.

All STM32 Nucleo boards are delivered with the solder-bridges configured according to the target supported STM32.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

### 7.12 Extension connectors

Figure 9 to Figure 25 show the signals connected by default to ARDUINO ®  Uno V3 connectors (CN5, CN6, CN8, CN9) and to ST morpho connector (CN7 and CN10), for each STM32 Nucleo board.

Figure 9. NUCLEO-F030R8

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_c717a60d9bce0bbbcb7c5c6aaeea2db86359eace878fd3e0b7557c6373738dd4.png)

Figure 10. NUCLEO-F070RB

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_36527f86cab7c14c6736b7ce28ab13dd8ca67932778df578c9f0808e1091591e.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_fa67eb8f66eab24ad09d37221a5cc6eca8e8ec349edfc8b9f421018d8037962f.png)

90

Figure 11. NUCLEO-F072RB

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_a6c15c33e227ecac58b71cee50da19c2b086260dad9d5f1a556dba362af2c937.png)

Figure 12. NUCLEO-F091RC

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_28f01cdacfbdc40416dc7e36128843a09eee54b13cb4c3d241e7b236122709ee.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_508f29889d800294755019c4ccea566eb851242ad15821598f4418edf88cbf53.png)

Figure 13. NUCLEO-F103RB

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_10a3c7c8a57401f74ec30ccda8e251dab0d05ba4f07377e6e1aba83473086a7b.png)

Figure 14. NUCLEO-F302R8

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_aa74060450563c69d7b2acf1faf9fcd92103a4dd9bc26167e5af4db522a8d2c0.png)

90

Figure 15. NUCLEO-F303RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_e3395fc4d52dd53202bb1eba93f540bf352e668601a8f8812a08ebc558ebee34.png)

Figure 16. NUCLEO-F334R8

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_504764d9d60fcafca04ebe92a1d92ea7481aa58a5ac2245313ab620a2bc946cf.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_508f29889d800294755019c4ccea566eb851242ad15821598f4418edf88cbf53.png)

Figure 17. NUCLEO-F401RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_09a6cc6c868c6d1ada36486f877836b3c398c9a01055dddf42660792071c4855.png)

Figure 18. NUCLEO-F411RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_30b3e4dae9bdac6eefc4bfd2d816a301d1978235645c75245fd714eace404594.png)

90

Figure 19. NUCLEO-L053R8

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_a5ed68f8786ac24bd7a7ec4a7b670dec524d86bc0eed99a265ca2208f1567b8f.png)

Figure 20. NUCLEO-L073RZ and NUCLEO-L010RB

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_1030f34275f6e4f5a67bb40a83bbe1272bb0ea87b682321d6ec996e271f456df.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_508f29889d800294755019c4ccea566eb851242ad15821598f4418edf88cbf53.png)

Figure 21. NUCLEO-L152RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_d90cc3907d07989a09b5f198e7004aea4f5b915ae0a2a92183cd4dc465cb54af.png)

Figure 22. NUCLEO-L452RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_ef6e4799b64850419c28d67526baa15f40cd09849465a7f358243532ad0656d2.png)

90

Figure 23. NUCLEO-L476RG

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_c859b5bfceb0dbbc2881dfa3a9a2640bb09a50fa88de54938ade14c7f283a258.png)

Figure 24. NUCLEO-F446RE

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_adf89c68af2e815088374797a07787391eb020b0844e81fa916890eb06da2e55.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000002_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_0d6ece4667210e157d4abe0cd6d8c4d19c7a17e6951619fbbe9bba64eecbbead.png)

Figure 25. NUCLEO-F410RB

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000001_26f115c63b7474ca1979af82e1b4934bf8164502a27dd271b84f6e56f93e9ba0.png)

90

### 7.13 ARDUINO ®  connectors

CN5, CN6, CN8, and CN9 are female connectors compatible with ARDUINO ®  standard. Most shields designed for ARDUINO ®  can fit the STM32 Nucleo boards.

The ARDUINO ®  connectors on the STM32 Nucleo board support the ARDUINO ®  Uno V3.

For compatibility with ARDUINO ®  Uno V1, apply the following modifications:

- SB46 and SB52 must be ON,
- SB51 and SB56 must be OFF to connect I 2 C on A4 (pin 5) and A5 (pin 6 of CN8).
- Caution 1: The I/Os of the STM32 microcontroller are 3.3 V compatible instead of 5 V for ARDUINO ® Uno V3.
- Caution 2: SB57 must be removed before implementing the ARDUINO ®  shield with VREF+ power being          provided on CN5 pin 8. Refer to Table 10: Solder bridges for details on SB57.

Table 11 to Table 23 show the pin assignment of each main STM32 microcontroller on ARDUINO ®  connectors.

Table 11. ARDUINO ®  connectors on NUCLEO-F030R8, NUCLEO-F070RB, NUCLEO-F072RB, NUCLEO-F091RC

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC_IN0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC_IN1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC_IN4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC_IN8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                         |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                         |
| CN5 digital      | 8                | AREF             | -                | AVDD                             |
| CN5 digital      | 7                | GND              | -                | ground                           |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

Table 11. ARDUINO ®  connectors on NUCLEO-F030R8, NUCLEO-F070RB, NUCLEO-F072RB, NUCLEO-F091RC (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function               |
|-------------|-------|------------|-------------|------------------------|
| CN5 digital |     6 | D13        | PA5         | SPI1_SCK               |
| CN5 digital |     5 | D12        | PA6         | SPI1_MISO              |
| CN5 digital |     4 | D11        | PA7         | TIM17_CH1 or SPI1_MOSI |
| CN5 digital |     3 | D10        | PB6         | TIM16_CH1N or SPI1_CS  |
| CN5 digital |     2 | D9         | PC7         | TIM3_CH2               |
| CN5 digital |     1 | D8         | PA9         | -                      |
| CN9 digital |     8 | D7         | PA8         | -                      |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3 (2)           |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1               |
| CN9 digital |     5 | D4         | PB5         | -                      |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2 (3)           |
| CN9 digital |     3 | D2         | PA10        | -                      |
| CN9 digital |     2 | D1         | PA2         | USART2_TX              |
| CN9 digital |     1 | D0         | PA3         | USART2_RX              |

2. PWM is not supported by D6 on STM32F030 and STM32F070 since the timer is not available on PB10.

3. PWM is not supported by D3 on STM32F030 and STM32F070 since the timer is not available on PB3.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_dffcf771ee0726e68112c8e4d084b946431cf3fd25c5ff0cf89856ebc72e7f93.png)

90

Table 12. ARDUINO ®  connectors on NUCLEO-F103RB

| Connector        | Pin              | Pin name         | STM32 pin        | Function                       |
|------------------|------------------|------------------|------------------|--------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                |
| CN6 power        | 1                | NC               | -                | -                              |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                       |
| CN6 power        | 3                | RESET            | NRST             | RESET                          |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output              |
| CN6 power        | 5                | +5V              |                  | 5V output                      |
| CN6 power        | 6                | GND              |                  | ground                         |
| CN6 power        | 7                | GND              |                  | ground                         |
| CN6 power        | 8                | VIN              |                  | Power input                    |
| CN8 analog       | 1                | A0               | PA0              | ADC_0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC_1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC_4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC_8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors               |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                       |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                       |
| CN5 digital      | 8                | AREF             |                  | AVDD                           |
| CN5 digital      | 7                | GND              | -                | ground                         |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                       |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                      |
| CN5 digital      | 4                | D11              | PA7              | TIM3_CH2 or SPI1_MOSI          |
| CN5 digital      | 3                | D10              | PB6              | TIM4_CH1 or SPI1_CS            |
| CN5 digital      | 2                | D9               | PC7              | TIM3_CH2                       |
| CN5 digital      | 1                | D8               | PA9              | -                              |
| CN9 digital      | 8                | D7               | PA8              |                                |
| CN9 digital      | 7                | D6               | PB10             | TIM2_CH3                       |
| CN9 digital      | 6                | D5               | PB4              | TIM3_CH1                       |
| CN9 digital      | 5                | D4               | PB5              | -                              |
| CN9 digital      | 4                | D3               | PB3              | TIM2_CH2                       |
| CN9 digital      | 3                | D2               | PA10             | -                              |
| CN9 digital      | 2                | D1               | PA2              | USART2_TX                      |
| CN9 digital      | 1                | D0               | PA3              | USART2_RX                      |

1. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5b6f804002fa02689696dc4242a93c368031f6bcc06803d823ad75ba9f5a7508.png)

Table 13. ARDUINO ®  connectors on NUCLEO-F302R8

| Connector        | Pin              | Pin name         | STM32 pin        | Function                        |
|------------------|------------------|------------------|------------------|---------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                 |
| CN6 power        | 1                | NC               | -                | -                               |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                        |
| CN6 power        | 3                | RESET            | NRST             | RESET                           |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output               |
| CN6 power        | 5                | +5V              | -                | 5V output                       |
| CN6 power        | 6                | GND              | -                | ground                          |
| CN6 power        | 7                | GND              | -                | ground                          |
| CN6 power        | 8                | VIN              | -                | Power input                     |
| CN8 analog       | 1                | A0               | PA0              | ADC_IN1                         |
| CN8 analog       | 2                | A1               | PA1              | ADC_IN2                         |
| CN8 analog       | 3                | A2               | PA4              | ADC_IN5                         |
| CN8 analog       | 4                | A3               | PB0              | ADC_IN11                        |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN7 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN6 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                        |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                        |
| CN5 digital      | 8                | AREF             | -                | AVDD                            |
| CN5 digital      | 7                | GND              | -                | ground                          |
| CN5 digital      | 6                | D13              | PB13             | SPI2_SCK                        |
| CN5 digital      | 5                | D12              | PB14             | SPI2_MISO                       |
| CN5 digital      | 4                | D11              | PB15             | TIM15_CH2 or SPI2_MOSI          |
| CN5 digital      | 3                | D10              | PB6              | TIM16_CH1N or SPI2_CS           |
| CN5 digital      | 2                | D9               | PC7              | -                               |
| CN5 digital      | 1                | D8               | PA9              | -                               |
| CN9 digital      | 8                | D7               | PA8              | -                               |
| CN9 digital      | 7                | D6               | PB10             | TIM2_CH3                        |
| CN9 digital      | 6                | D5               | PB4              | TIM16_CH1                       |
| CN9 digital      | 5                | D4               | PB5              | -                               |
| CN9 digital      | 4                | D3               | PB3              | TIM2_CH2                        |
| CN9 digital      | 3                | D2               | PA10             | -                               |
| CN9 digital      | 2                | D1               | PA2              | USART2_TX                       |
| CN9 digital      | 1                | D0               | PA3              | USART2_RX                       |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_0d6ece4667210e157d4abe0cd6d8c4d19c7a17e6951619fbbe9bba64eecbbead.png)

90

#### Warning: PWM is not supported by D9 on STM32F302 since the timer is not available on PC7.

Table 14. ARDUINO ®  connectors on NUCLEO-F303RE

| Connector        | Pin              | Pin name         | STM32 pin        | Function                          |
|------------------|------------------|------------------|------------------|-----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                   |
| CN6 power        | 1                | NC               | -                | -                                 |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                          |
| CN6 power        | 3                | RESET            | NRST             | RESET                             |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                 |
| CN6 power        | 5                | +5V              | -                | 5V output                         |
| CN6 power        | 6                | GND              | -                | ground                            |
| CN6 power        | 7                | GND              | -                | ground                            |
| CN6 power        | 8                | VIN              | -                | Power input                       |
| CN8 analog       | 1                | A0               | PA0              | ADC1_IN1                          |
| CN8 analog       | 2                | A1               | PA1              | ADC1_IN2                          |
| CN8 analog       | 3                | A2               | PA4              | ADC2_IN1                          |
| CN8 analog       | 4                | A3               | PB0              | ADC3_IN12                         |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC12_IN7 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC12_IN6 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                  |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                          |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                          |
| CN5 digital      | 8                | AREF             | -                | AVDD                              |
| CN5 digital      | 7                | GND              | -                | ground                            |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                          |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                         |
| CN5 digital      | 4                | D11              | PA7              | TIM17_CH1 or SPI1_MOSI            |
| CN5 digital      | 3                | D10              | PB6              | TIM4_CH1 or SPI1_CS               |
| CN5 digital      | 2                | D9               | PC7              | TIM3_CH2                          |
| CN5 digital      | 1                | D8               | PA9              | -                                 |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bc45435175705838744f49985111e53d1311c083e6553a5c3f18e9826d958a1.png)

Table 14. ARDUINO ®  connectors on NUCLEO-F303RE (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN9 digital |     8 | D7         | PA8         | -          |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3   |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1   |
| CN9 digital |     5 | D4         | PB5         | -          |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |

1. Refer to Table 10: Solder bridges for details.

Table 15. ARDUINO ®  connectors on NUCLEO-F334R8

| Connector        | Pin              | Pin name         | STM32 pin        | Function                        |
|------------------|------------------|------------------|------------------|---------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                 |
| CN6 power        | 1                | NC               | -                | -                               |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                        |
| CN6 power        | 3                | RESET            | NRST             | RESET                           |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output               |
| CN6 power        | 5                | +5V              | -                | 5V output                       |
| CN6 power        | 6                | GND              | -                | ground                          |
| CN6 power        | 7                | GND              | -                | ground                          |
| CN6 power        | 8                | VIN              | -                | Power input                     |
| CN8 analog       | 1                | A0               | PA0              | ADC1_IN1                        |
| CN8 analog       | 2                | A1               | PA1              | ADC1_IN2                        |
| CN8 analog       | 3                | A2               | PA4              | ADC2_IN1                        |
| CN8 analog       | 4                | A3               | PB0              | ADC1_IN11                       |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN7 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN6 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                        |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                        |
| CN5 digital      | 8                | AREF             | -                | AVDD                            |
| CN5 digital      | 7                | GND              | -                | ground                          |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                        |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                       |
| CN5 digital      | 4                | D11              | PA7              | TIM17_CH1 or SPI1_MOSI          |
| CN5 digital      | 3                | D10              | PB6              | TIM16_CH1N or SPI1_CS           |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_f386b6b71d0f32a6f0b26262c8a710a2bef907fd97a6453c8730f974cb441d53.png)

90

Table 15. ARDUINO ®  connectors on NUCLEO-F334R8 (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN5 digital |     2 | D9         | PC7         | TIM3_CH2   |
| CN5 digital |     1 | D8         | PA9         | -          |
| CN9 digital |     8 | D7         | PA8         | -          |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3   |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1   |
| CN9 digital |     5 | D4         | PB5         | -          |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |
| CN9 digital |     1 | D0         | PA3         | USART2_RX  |

1. Refer to Table 10: Solder bridges for details.

Table 16. ARDUINO ®  connectors on NUCLEO-F401RE and NUCLEO-F411RE

| Connector        | Pin              | Pin name         | STM32 pin        | Function                        |
|------------------|------------------|------------------|------------------|---------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                 |
| CN6 power        | 1                | NC               | -                | -                               |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                        |
| CN6 power        | 3                | RESET            | NRST             | RESET                           |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output               |
| CN6 power        | 5                | +5V              | -                | 5V output                       |
| CN6 power        | 6                | GND              | -                | ground                          |
| CN6 power        | 7                | GND              | -                | ground                          |
| CN6 power        | 8                | VIN              | -                | Power input                     |
| CN8 analog       | 1                | A0               | PA0              | ADC1_0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC1_1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC1_4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC1_8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC1_11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC1_10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                        |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                        |
| CN5 digital      | 8                | AREF             | -                | AVDD                            |
| CN5 digital      | 7                | GND              | -                | ground                          |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

Table 16. ARDUINO ®  connectors on NUCLEO-F401RE and NUCLEO-F411RE (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function               |
|-------------|-------|------------|-------------|------------------------|
| CN5 digital |     6 | D13        | PA5         | SPI1_SCK               |
| CN5 digital |     5 | D12        | PA6         | SPI1_MISO              |
| CN5 digital |     4 | D11        | PA7         | TIM1_CH1N or SPI1_MOSI |
| CN5 digital |     3 | D10        | PB6         | TIM4_CH1 or SPI1_CS    |
| CN5 digital |     2 | D9         | PC7         | TIM3_CH2               |
| CN5 digital |     1 | D8         | PA9         | -                      |
| CN9 digital |     8 | D7         | PA8         | -                      |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3               |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1               |
| CN9 digital |     5 | D4         | PB5         | -                      |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2               |
| CN9 digital |     3 | D2         | PA10        | -                      |
| CN9 digital |     2 | D1         | PA2         | USART2_TX              |
| CN9 digital |     1 | D0         | PA3         | USART2_RX              |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_04216e8fa57c8b9aa0e8f0a66f5b8df565eb96f4348fee201d8e5f91cd96a94c.png)

90

Table 17. ARDUINO ®  connectors on NUCLEO-L053R8

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC_IN0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC_IN1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC_IN4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC_IN8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                         |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                         |
| CN5 digital      | 8                | AREF             | -                | AVDD                             |
| CN5 digital      | 7                | GND              | -                | ground                           |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                         |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                        |
| CN5 digital      | 4                | D11              | PA7              | TIM12_CH2 or SPI1_MOSI           |
| CN5 digital      | 3                | D10              | PB6              | SPI1_CS                          |
| CN5 digital      | 2                | D9               | PC7              | TIM12_CH2                        |
| CN5 digital      | 1                | D8               | PA9              | -                                |
| CN9 digital      | 8                | D7               | PA8              | -                                |
| CN9 digital      | 7                | D6               | PB10             | TIM2_CH3                         |
| CN9 digital      | 6                | D5               | PB4              | TIM12_CH1                        |
| CN9 digital      | 5                | D4               | PB5              | -                                |
| CN9 digital      | 4                | D3               | PB3              | TIM2_CH2                         |
| CN9 digital      | 3                | D2               | PA10             | -                                |
| CN9 digital      | 2                | D1               | PA2              | USART2_TX                        |
| CN9 digital      | 1                | D0               | PA3              | USART2_RX                        |

1. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_12ddcae8d3f468a9de0eae74b408c651d21b16e49e97da681fad642a47224247.png)

#### Warning: PWM is not supported by D10 on STM32L053 since the timer is not

available on PB6.

Table 18. ARDUINO ®  connectors on NUCLEO-L010RB and NUCLEO-L073RZ

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC_IN0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC_IN1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC_IN4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC_IN8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                         |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                         |
| CN5 digital      | 8                | AREF             | -                | AVDD                             |
| CN5 digital      | 7                | GND              | -                | ground                           |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                         |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                        |
| CN5 digital      | 4                | D11              | PA7              | TIM22_CH2 or SPI1_MOSI           |
| CN5 digital      | 3                | D10              | PB6              | SPI1_CS                          |
| CN5 digital      | 2                | D9               | PC7              | TIM3_CH2                         |
| CN5 digital      | 1                | D8               | PA9              | -                                |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_d3556ae52a3a50bc5756c5da879ccfd024de16a8e933077aeec537bf93c3a741.png)

90

Table 18. ARDUINO ®  connectors on NUCLEO-L010RB and NUCLEO-L073RZ (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN9 digital |     8 | D7         | PA8         | -          |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3   |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1   |
| CN9 digital |     5 | D4         | PB5         | -          |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |
| CN9 digital |     1 | D0         | PA3         | USART2_RX  |

Warning:

PWM is not supported by D10 on STM32L010 and STM32L073 since the timer is not available on PB6.

Table 19. ARDUINO ®  connectors on NUCLEO-F446RE

| Connector        | Pin              | Pin name         | STM32 pin        | Function                            |
|------------------|------------------|------------------|------------------|-------------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                     |
| CN6 power        | 1                | NC               | -                | -                                   |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                            |
| CN6 power        | 3                | RESET            | NRST             | RESET                               |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                   |
| CN6 power        | 5                | +5V              | -                | 5V output                           |
| CN6 power        | 6                | GND              | -                | ground                              |
| CN6 power        | 7                | GND              | -                | ground                              |
| CN6 power        | 8                | VIN              | -                | Power input                         |
| CN8 analog       | 1                | A0               | PA0              | ADC123_IN0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC123_IN1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC12_IN4                           |
| CN8 analog       | 4                | A3               | PB0              | ADC12_IN8                           |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC123_IN11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8(1)    | ADC123_IN10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                    |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                            |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                            |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

Table 19. ARDUINO ®  connectors on NUCLEO-F446RE (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function                         |
|-------------|-------|------------|-------------|----------------------------------|
| CN5 digital |     8 | AREF       | -           | AVDD                             |
| CN5 digital |     7 | GND        | -           | ground                           |
| CN5 digital |     6 | D13        | PA5         | SPI1_SCK                         |
| CN5 digital |     5 | D12        | PA6         | SPI1_MISO                        |
| CN5 digital |     4 | D11        | PA7         | TIM14_CH1 &#124;&#124; SPI1_MOSI |
| CN5 digital |     3 | D10        | PB6         | TIM4_CH1 &#124;&#124; SPI1_CS    |
| CN5 digital |     2 | D9         | PC7         | TIM8_CH2                         |
| CN5 digital |     1 | D8         | PA9         | -                                |
| CN9 digital |     8 | D7         | PA8         | -                                |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3                         |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1                         |
| CN9 digital |     5 | D4         | PB5         | -                                |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2                         |
| CN9 digital |     3 | D2         | PA10        | -                                |
| CN9 digital |     2 | D1         | PA2         | USART2_TX                        |
| CN9 digital |     1 | D0         | PA3         | USART2_RX                        |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_04216e8fa57c8b9aa0e8f0a66f5b8df565eb96f4348fee201d8e5f91cd96a94c.png)

90

Table 20. ARDUINO ®  connectors on NUCLEO-F410RB

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC1_0                           |
| CN8 analog       | 2                | A1               | PA1              | ADC1_1                           |
| CN8 analog       | 3                | A2               | PA4              | ADC1_4                           |
| CN8 analog       | 4                | A3               | PB0              | ADC1_8                           |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC1_11 (PC1) or I2C1_SDA (PB9)  |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC1_10 (PC0) or I2C1_SCL (PB8)  |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | PB8              | D15              | I2C1_SCL                         |
| CN5 digital      | 9                | PB9              | D14              | I2C1_SDA                         |
| CN5 digital      | 8                | -                | AREF             | AVDD                             |
| CN5 digital      | 7                | -                | GND              | ground                           |
| CN5 digital      | 6                | PA5              | D13              | SPI1_SCK                         |
| CN5 digital      | 5                | PA6              | D12              | SPI1_MISO                        |
| CN5 digital      | 4                | PA7              | D11              | TIM1_CH1N &#124;&#124; SPI1_MOSI |
| CN5 digital      | 3                | PB6              | D10              | SPI1_CS                          |
| CN5 digital      | 2                | PC7              | D9               | -                                |
| CN5 digital      | 1                | PA9              | D8               | -                                |
| CN9 digital      | 8                | PA8              | D7               | -                                |
| CN9 digital      | 7                | PB10             | D6               | -                                |
| CN9 digital      | 6                | PB4              | D5               | -                                |
| CN9 digital      | 5                | PB5              | D4               | -                                |
| CN9 digital      | 4                | PB3              | D3               | -                                |
| CN9 digital      | 3                | PA10             | D2               | -                                |
| CN9 digital      | 2                | PA2              | D1               | USART2_TX                        |
| CN9 digital      | 1                | PA3              | D0               | USART2_RX                        |

1. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_a0526f7c3d171e9ffdd878ba20cfec3cb0c2f5eb01b811ad50c1119db5c3a701.png)

#### Warning: PWM is not supported by D3, D5, D6, D9 and D10 on STM32F410RB since timer is not available on PB6, PC7, PB10, PB4, PB3.

Table 21. ARDUINO ®  connectors on NUCLEO-L152RE

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC_IN0                          |
| CN8 analog       | 2                | A1               | PA1              | ADC_IN1                          |
| CN8 analog       | 3                | A2               | PA4              | ADC_IN4                          |
| CN8 analog       | 4                | A3               | PB0              | ADC_IN8                          |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC_IN11 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC_IN10 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                         |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                         |
| CN5 digital      | 8                | AREF             | -                | AVDD                             |
| CN5 digital      | 7                | GND              | -                | ground                           |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                         |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                        |
| CN5 digital      | 4                | D11              | PA7              | TIM11_CH1 or SPI1_MOSI           |
| CN5 digital      | 3                | D10              | PB6              | TIM4_CH1 or SPI1_CS              |
| CN5 digital      | 2                | D9               | PC7              | TIM3_CH2                         |
| CN5 digital      | 1                | D8               | PA9              | -                                |
| CN9 digital      | 8                | D7               | PA8              | -                                |
| CN9 digital      | 7                | D6               | PB10             | TIM2_CH3                         |
| CN9 digital      | 6                | D5               | PB4              | TIM3_CH1                         |
| CN9 digital      | 5                | D4               | PB5              | -                                |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_0d6ece4667210e157d4abe0cd6d8c4d19c7a17e6951619fbbe9bba64eecbbead.png)

90

Table 21. ARDUINO ®  connectors on NUCLEO-L152RE (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |
| CN9 digital |     1 | D0         | PA3         | USART2_RX  |

1. Refer to Table 10: Solder bridges for details.

Table 22. ARDUINO ®  connectors on NUCLEO-L452RE

| Connector        | Pin              | Pin name         | STM32 pin        | Function                         |
|------------------|------------------|------------------|------------------|----------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                  |
| CN6 power        | 1                | NC               | -                | -                                |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                         |
| CN6 power        | 3                | RESET            | NRST             | RESET                            |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                |
| CN6 power        | 5                | +5V              | -                | 5V output                        |
| CN6 power        | 6                | GND              | -                | ground                           |
| CN6 power        | 7                | GND              | -                | ground                           |
| CN6 power        | 8                | VIN              | -                | Power input                      |
| CN8 analog       | 1                | A0               | PA0              | ADC1_IN5                         |
| CN8 analog       | 2                | A1               | PA1              | ADC1_IN6                         |
| CN8 analog       | 3                | A2               | PA4              | ADC1_IN9                         |
| CN8 analog       | 4                | A3               | PB0              | ADC1_IN15                        |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC1_IN2 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC1_IN1 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                 |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                         |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                         |
| CN5 digital      | 8                | AREF             | -                | AVDD                             |
| CN5 digital      | 7                | GND              | -                | ground                           |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                         |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                        |
| CN5 digital      | 4                | D11              | PA7              | TIM1_CH1N or SPI1_MOSI           |
| CN5 digital      | 3                | D10              | PB6              | TIM16_CH1N or SPI1_CS            |
| CN5 digital      | 2                | D9               | PC7              | TIM3_CH2                         |
| CN5 digital      | 1                | D8               | PA9              | -                                |
| CN9 digital      | 8                | D7               | PA8              | -                                |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

Table 22. ARDUINO ®  connectors on NUCLEO-L452RE (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3   |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1   |
| CN9 digital |     5 | D4         | PB5         | -          |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |
| CN9 digital |     1 | D0         | PA3         | USART2_RX  |

Table 23. ARDUINO ®  connectors on NUCLEO-L476RG

| Connector        | Pin              | Pin name         | STM32 pin        | Function                           |
|------------------|------------------|------------------|------------------|------------------------------------|
| Left connectors  | Left connectors  | Left connectors  | Left connectors  | Left connectors                    |
| CN6 power        | 1                | NC               | -                | -                                  |
| CN6 power        | 2                | IOREF            | -                | 3.3V Ref                           |
| CN6 power        | 3                | RESET            | NRST             | RESET                              |
| CN6 power        | 4                | +3.3V            | -                | 3.3V input/output                  |
| CN6 power        | 5                | +5V              | -                | 5V output                          |
| CN6 power        | 6                | GND              | -                | ground                             |
| CN6 power        | 7                | GND              | -                | ground                             |
| CN6 power        | 8                | VIN              | -                | Power input                        |
| CN8 analog       | 1                | A0               | PA0              | ADC12_IN5                          |
| CN8 analog       | 2                | A1               | PA1              | ADC12_IN6                          |
| CN8 analog       | 3                | A2               | PA4              | ADC12_IN9                          |
| CN8 analog       | 4                | A3               | PB0              | ADC12_IN15                         |
| CN8 analog       | 5                | A4               | PC1 or PB9 (1)   | ADC123_IN2 (PC1) or I2C1_SDA (PB9) |
| CN8 analog       | 6                | A5               | PC0 or PB8 (1)   | ADC123_IN1 (PC0) or I2C1_SCL (PB8) |
| Right connectors | Right connectors | Right connectors | Right connectors | Right connectors                   |
| CN5 digital      | 10               | D15              | PB8              | I2C1_SCL                           |
| CN5 digital      | 9                | D14              | PB9              | I2C1_SDA                           |
| CN5 digital      | 8                | AREF             | -                | AVDD                               |
| CN5 digital      | 7                | GND              | -                | ground                             |
| CN5 digital      | 6                | D13              | PA5              | SPI1_SCK                           |
| CN5 digital      | 5                | D12              | PA6              | SPI1_MISO                          |
| CN5 digital      | 4                | D11              | PA7              | TIM17_CH1 or SPI1_MOSI             |
| CN5 digital      | 3                | D10              | PB6              | TIM4_CH1 or SPI1_CS                |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_eb82e85fa3448104d62ff6e7352d610cab60e4e2905a733214be23971d14c303.png)

90

Table 23. ARDUINO ®  connectors on NUCLEO-L476RG (continued)

| Connector   |   Pin | Pin name   | STM32 pin   | Function   |
|-------------|-------|------------|-------------|------------|
| CN5 digital |     2 | D9         | PC7         | TIM3_CH2   |
| CN5 digital |     1 | D8         | PA9         | -          |
| CN9 digital |     8 | D7         | PA8         | -          |
| CN9 digital |     7 | D6         | PB10        | TIM2_CH3   |
| CN9 digital |     6 | D5         | PB4         | TIM3_CH1   |
| CN9 digital |     5 | D4         | PB5         | -          |
| CN9 digital |     4 | D3         | PB3         | TIM2_CH2   |
| CN9 digital |     3 | D2         | PA10        | -          |
| CN9 digital |     2 | D1         | PA2         | USART2_TX  |
| CN9 digital |     1 | D0         | PA3         | USART2_RX  |

1. Refer to Table 10: Solder bridges for details.

### 7.14 ST morpho connector

The ST morpho connector consists of male pin headers (CN7 and CN10) accessible on both sides of the board. They can be used to connect the STM32 Nucleo board to an extension board or a prototype/wrapping board placed on the top or bottom side of the STM32 Nucleo board. All signals and power pins of the STM32 are available on the ST morpho connector. This connector can also be probed by an oscilloscope, logical analyzer, or voltmeter.

Table 24 to Table 33 show the pin assignments of each STM32 on the ST morpho connector.

Table 24. ST morpho connector on NUCLEO-F030R8

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | PF6            | -               | 10              | 9               | GND             | -                | 10               |
| 11             | PF7            | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13           | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14           | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_5103c6011e32f247e6c384128704d20c28fae121d360143e9fe6a291c3b9e605.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_9b3a127f359f96dc35ea3991b8ab5df94d5b0b8e3a2b5b37e5df292d21baff66.png)

Table 24. ST morpho connector on NUCLEO-F030R8 (continued)

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 23             | PC13 (3)       | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14 (3)       | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PF0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PF1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | PF5              | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | PF4              | 38               |

2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.
1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pin5-7 of CN7.
2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.

Table 25. ST morpho connector on NUCLEO-F070RB

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PF0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PF1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VDD            | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

90

3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.
2. CN7 pin 7 (BOOT0) can be configured by engineering byte as PF11 on NUCLEO-F091RC.
3. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
4. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
5. Refer to Table 10: Solder bridges for details.

Table 26. ST morpho connector on NUCLEO-F072RB, NUCLEO-F091RC, NUCLEO-F303RE, NUCLEO-F334R8

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)(2)   | GND             | 8               | 7               | AVDD            | U5V (3)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (4)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (4)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PF0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PF1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (5)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (5)  | 38              | 37              | PA3             | -                | 38               |

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bc45435175705838744f49985111e53d1311c083e6553a5c3f18e9826d958a1.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_9b3a127f359f96dc35ea3991b8ab5df94d5b0b8e3a2b5b37e5df292d21baff66.png)

Table 27. ST morpho connector on NUCLEO-F103RB

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PD0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PD1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5 V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

90

Table 28. ST morpho connector on NUCLEO-F302R8

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PB13            | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PB14            | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PB15            | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PA7              | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PA6              | 28               |
| 29             | PF0            | PA1             | 30              | 29              | PB5             | PA5              | 30               |
| 31             | PF1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pin5-7 of CN7. Two unused jumpers are available on CN11 and CN12 (bottom side of the board).
2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bc45435175705838744f49985111e53d1311c083e6553a5c3f18e9826d958a1.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_dffcf771ee0726e68112c8e4d084b946431cf3fd25c5ff0cf89856ebc72e7f93.png)

Table 29. ST morpho connector on NUCLEO-F401RE, NUCLEO-F411RE, NUCLEO-F446RE

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | -                | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PH0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PH1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

90

Table 30. ST morpho connector on NUCLEO-L010RB, NUCLEO-L053R8, NUCLEO-L073RZ, NUCLEO-L152RE

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PH0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PH1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VLCD           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pin5-7 of CN7. Two unused jumpers are available on CN11 and CN12 (bottom side of the board).
2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bc45435175705838744f49985111e53d1311c083e6553a5c3f18e9826d958a1.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_fa67eb8f66eab24ad09d37221a5cc6eca8e8ec349edfc8b9f421018d8037962f.png)

Table 31. ST morpho connector on NUCLEO-L452RE

| CN7 odd pins   | CN7 odd pins    | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|-----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name            | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10            | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12            | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD             | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | PH3 / BOOT0 (1) | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -               | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -               | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)        | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)        | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15            | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND             | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7             | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13            | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14            | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15            | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PH0             | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PH1             | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT            | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2             | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3             | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pins 5-7 of CN7.
2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

90

Table 32. ST morpho connector on NUCLEO-L476RG

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PD2             | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | PB11             | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PH0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PH1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pin5-7 of CN7.
2. U5V is 5 V power from ST-LINK/V2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_8bc45435175705838744f49985111e53d1311c083e6553a5c3f18e9826d958a1.png)

![Image](../um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics-1_artifacts/image_000000_fa67eb8f66eab24ad09d37221a5cc6eca8e8ec349edfc8b9f421018d8037962f.png)

Table 33. ST morpho connector on NUCLEO-F410RB

| CN7 odd pins   | CN7 odd pins   | CN7 even pins   | CN7 even pins   | CN10 odd pins   | CN10 odd pins   | CN10 even pins   | CN10 even pins   |
|----------------|----------------|-----------------|-----------------|-----------------|-----------------|------------------|------------------|
| Pin            | Name           | Name            | Pin             | Pin             | Name            | Name             | Pin              |
| 1              | PC10           | PC11            | 2               | 1               | PC9             | PC8              | 2                |
| 3              | PC12           | PB11            | 4               | 3               | PB8             | PC6              | 4                |
| 5              | VDD            | E5V             | 6               | 5               | PB9             | PC5              | 6                |
| 7              | BOOT0 (1)      | GND             | 8               | 7               | AVDD            | U5V (2)          | 8                |
| 9              | -              | -               | 10              | 9               | GND             | -                | 10               |
| 11             | -              | IOREF           | 12              | 11              | PA5             | PA12             | 12               |
| 13             | PA13 (3)       | RESET           | 14              | 13              | PA6             | PA11             | 14               |
| 15             | PA14 (3)       | +3.3V           | 16              | 15              | PA7             | PB12             | 16               |
| 17             | PA15           | +5V             | 18              | 17              | PB6             | -                | 18               |
| 19             | GND            | GND             | 20              | 19              | PC7             | GND              | 20               |
| 21             | PB7            | GND             | 22              | 21              | PA9             | PB2              | 22               |
| 23             | PC13           | VIN             | 24              | 23              | PA8             | PB1              | 24               |
| 25             | PC14           | -               | 26              | 25              | PB10            | PB15             | 26               |
| 27             | PC15           | PA0             | 28              | 27              | PB4             | PB14             | 28               |
| 29             | PH0            | PA1             | 30              | 29              | PB5             | PB13             | 30               |
| 31             | PH1            | PA4             | 32              | 31              | PB3             | AGND             | 32               |
| 33             | VBAT           | PB0             | 34              | 33              | PA10            | PC4              | 34               |
| 35             | PC2            | PC1 or PB9 (4)  | 36              | 35              | PA2             | -                | 36               |
| 37             | PC3            | PC0 or PB8 (4)  | 38              | 37              | PA3             | -                | 38               |

1. The default state of BOOT0 is LOW. It can be set to HIGH when a jumper is on pin5-7 of CN7.
2. U5V is 5 V power from the ST-LINKV2-1 USB connector and it rises before +5V.
3. PA13 and PA14 share with SWD signals connected to ST-LINK/V2-1, it is not recommended to use them as IO pins if the ST-LINK part is not cut.
4. Refer to Table 10: Solder bridges for details.

90
