/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*! @name PORTA19 (number 14), LED_G
  @{ */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_LED_GREEN_GPIO GPIOA                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_LED_GREEN_GPIO_PIN_MASK (1U << 19U) /*!<@brief GPIO pin mask */

/* Symbols to be used with PORT driver */
#define BOARD_INITPINS_LED_GREEN_PORT PORTA                /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_LED_GREEN_PIN 19U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_LED_GREEN_PIN_MASK (1U << 19U)      /*!<@brief PORT pin mask */
                                                           /* @} */

/*! @name PORTC1 (number 38), LPSPI_PCS3/LED_B
  @{ */
/* Symbols to be used with GPIO driver */
#define BOARD_INITPINLED1_LED1_GPIO GPIOC               /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINLED1_LED1_GPIO_PIN_MASK (1U << 1U) /*!<@brief GPIO pin mask */

/* Symbols to be used with PORT driver */
#define BOARD_INITPINLED1_LED1_PORT PORTC               /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINLED1_LED1_PIN 1U                   /*!<@brief PORT pin number */
#define BOARD_INITPINLED1_LED1_PIN_MASK (1U << 1U)      /*!<@brief PORT pin mask */
                                                        /* @} */
                                                           
/*!
 * @brief Configures green LED pin routing and optionally pin electrical features.
 *
 */
void knx_initPinGreenLed(void);
void knx_InitPinMonoLed(void);
                                                           