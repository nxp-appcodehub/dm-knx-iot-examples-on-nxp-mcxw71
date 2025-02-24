/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "knx_pin_mux.h"

void knx_initPinGreenLed(void)
{
    /* Clock Configuration: Peripheral clocks are enabled; module does not stall low power mode entry */
    CLOCK_EnableClock(kCLOCK_GpioA);
    /* Clock Configuration: Peripheral clocks are enabled; module does not stall low power mode entry */
    CLOCK_EnableClock(kCLOCK_PortA);

    gpio_pin_config_t LED_GREEN_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1U
    };
    /* Initialize GPIO functionality on pin PTA19 (pin 14)  */
    GPIO_PinInit(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_PIN, &LED_GREEN_config);

    const port_pin_config_t LED_GREEN = {/* Internal pull-up/down resistor is disabled */
                                         (uint16_t)kPORT_PullDisable,
                                         /* Low internal pull resistor value is selected. */
                                         (uint16_t)kPORT_LowPullResistor,
                                         /* Fast slew rate is configured */
                                         (uint16_t)kPORT_FastSlewRate,
                                         /* Passive input filter is disabled */
                                         (uint16_t)kPORT_PassiveFilterDisable,
                                         /* Open drain output is disabled */
                                         (uint16_t)kPORT_OpenDrainDisable,
                                         /* Low drive strength is configured */
                                         (uint16_t)kPORT_LowDriveStrength,
                                         /* Normal drive strength is configured */
                                         (uint16_t)kPORT_NormalDriveStrength,
                                         /* Pin is configured as PTA19 */
                                         (uint16_t)kPORT_MuxAsGpio,
                                         /* Pin Control Register fields [15:0] are not locked */
                                         (uint16_t)kPORT_UnlockRegister};
    /* PORTA19 (pin 14) is configured as PTA19 */
    PORT_SetPinConfig(BOARD_INITPINS_LED_GREEN_PORT, BOARD_INITPINS_LED_GREEN_PIN, &LED_GREEN);
}

void knx_InitPinMonoLed(void)
{
    /* Clock Configuration: Peripheral clocks are enabled; module does not stall low power mode entry */
    CLOCK_EnableClock(kCLOCK_GpioC);
    /* Clock Configuration: Peripheral clocks are enabled; module does not stall low power mode entry */
    CLOCK_EnableClock(kCLOCK_PortC);

    gpio_pin_config_t LED_MONO_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1U
    };
    /* Initialize GPIO functionality on pin PTA19 (pin 14)  */
    GPIO_PinInit(BOARD_INITPINLED1_LED1_GPIO, BOARD_INITPINLED1_LED1_PIN, &LED_MONO_config);

    const port_pin_config_t LED1_R = {/* Internal pull-up/down resistor is disabled */
                                      (uint16_t)kPORT_PullDisable,
                                      /* Low internal pull resistor value is selected. */
                                      (uint16_t)kPORT_LowPullResistor,
                                      /* Slow slew rate is configured */
                                      (uint16_t)kPORT_SlowSlewRate,
                                      /* Passive input filter is disabled */
                                      (uint16_t)kPORT_PassiveFilterDisable,
                                      /* Open drain output is disabled */
                                      (uint16_t)kPORT_OpenDrainDisable,
                                      /* Low drive strength is configured */
                                      (uint16_t)kPORT_LowDriveStrength,
                                      /* Normal drive strength is configured */
                                      (uint16_t)kPORT_NormalDriveStrength,
                                      /* Pin is configured as PTC0 */
                                      (uint16_t)kPORT_MuxAsGpio,
                                      /* Pin Control Register fields [15:0] are not locked */
                                      (uint16_t)kPORT_UnlockRegister};
    /* PORTC0 (pin 37) is configured as PTC0 */
    PORT_SetPinConfig(BOARD_INITPINLED1_LED1_PORT, BOARD_INITPINLED1_LED1_PIN, &LED1_R);
}