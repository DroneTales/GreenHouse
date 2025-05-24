#pragma once

/*****************************************************/
/*                 Build-in hardware                 */

#define SW_PB_PIN           GPIO_NUM_0

// UART0
#define USB_UART_TX_PIN     GPIO_NUM_1
#define USB_UART_RX_PIN     GPIO_NUM_3

// A7608H
#define A7608H_PWRKEY_PIN   GPIO_NUM_4
#define A7608H_RESET_PIN    GPIO_NUM_5
#define A7608H_DTR_PIN      GPIO_NUM_25
#define A7608H_TX_PIN       GPIO_NUM_26
#define A7608H_RX_PIN       GPIO_NUM_27
#define A7608H_RI_PIN       GPIO_NUM_33

// LiIon battery
#define BATT_18650_EN_PIN   GPIO_NUM_12
#define BATT_18650_ADC_PIN  GPIO_NUM_35

// Card reader
#define TF_CARD_MISO_PIN    GPIO_NUM_2
#define TF_CARD_CS_PIN      GPIO_NUM_13
#define TF_CARD_SCLK_PIN    GPIO_NUM_14
#define TF_CARD_MOSI_PIN    GPIO_NUM_15

// I2C
#define WIRE_SDA_PIN        GPIO_NUM_21
#define WIRE_SCL_PIN        GPIO_NUM_22


/*****************************************************/
/*                     Free pins                     */

// GPIO
#define ESP32_GPIO_18       GPIO_NUM_18
#define ESP32_GPIO_19       GPIO_NUM_19
#define ESP32_GPIO_23       GPIO_NUM_23
#define ESP32_GPIO_32       GPIO_NUM_32
#define ESP32_GPIO_34       GPIO_NUM_34
#define ESP32_GPIO_36       GPIO_NUM_36
#define ESP32_GPIO_39       GPIO_NUM_39

// Touch
#define ESP32_TOUCH_9       ESP32_GPIO32

// ADC
#define ESP32_ADC_02        ESP32_GPIO_36
#define ESP32_ADC_03        ESP32_GPIO_39
#define ESP32_ADC_04        ESP32_GPIO_32
#define ESP32_ADC_06        ESP32_GPIO_34

// ESP32 sensor (actualy the regular ADC)
#define ESP32_SENSOR_VP     ESP32_ADC_02
#define ESP32_SENSOR_VN     ESP32_ADC_03

// Partial free pins if I2C not used
#define ESP32_GPIO_21       WIRE_SDA_PIN
#define ESP32_GPIO_22       WIRE_SCL_PIN


/*****************************************************/
/*                 External hardware                 */

// Temperature sensor.
#define DS18B20_PIN         ESP32_GPIO_22
#define DS18B20_POWER_PIN   ESP32_GPIO_23
