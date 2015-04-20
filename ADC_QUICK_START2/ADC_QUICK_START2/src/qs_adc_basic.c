/**
 * \file
 *
 * \brief SAM ADC Quick Start
 *
 * Copyright (C) 2013-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include "conf_sio2host.h"

void configure_adc(void);

//! [module_inst]
struct adc_module adc_instance;
//! [module_inst]

//! [setup]
void configure_adc(void)
{
//! [setup_config]
	struct adc_config config_adc;
//! [setup_config]


//! [setup_config_defaults]
	adc_get_config_defaults(&config_adc);
//! [setup_config_defaults]

config_adc.reference =  ADC_REFERENCE_INT1V;	//ADC_REFERENCE_INTVCC0;		//reference voltage on pin 9: PA04
config_adc.resolution = ADC_RESOLUTION_12BIT;			//12 bit resolution
config_adc.divide_result = ADC_DIVIDE_RESULT_DISABLE;	//Don't divide result register after accumulation
config_adc.positive_input = ADC_POSITIVE_INPUT_PIN10;	//voltage positive input on pin 10: PA05
config_adc.negative_input = ADC_NEGATIVE_INPUT_PIN7;	//voltage negative input as internal ground : PA06

//! [setup_set_config]
	adc_init(&adc_instance, ADC, &config_adc);
//! [setup_set_config]


//! [setup_enable]
	adc_enable(&adc_instance);
//! [setup_enable]
}
//! [setup]

static struct usart_module cdc_uart_module;

static void configure_console(void)
{
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = HOST_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = HOST_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = HOST_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = HOST_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = HOST_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = USART_HOST_BAUDRATE;

	stdio_serial_init(&cdc_uart_module, USART_HOST, &usart_conf);
	usart_enable(&cdc_uart_module);
}

int main(void)
{
	system_init();
	delay_init();
	
//! [setup_init]
	configure_adc();
//! [setup_init]

//! [main]
//! [start_conv]
	adc_start_conversion(&adc_instance);
//! [start_conv]

//! [get_res]
	uint16_t result=0;

	configure_console();
	

//! [get_res]

//! [inf_loop]
	while (1) {
		/* Infinite loop */
		//adc_read(&adc_instance, &result);
		do {
		/* Wait for conversion to be done and read out result */
		} while (adc_read(&adc_instance, &result) == STATUS_BUSY);
		printf("The result is %d\n",result);
		uint32_t far = 9.0/5.0*((float)result*.0002441406*6.0/.01)+32.0;
		printf(" The temp is %d", far);
		adc_clear_status(&adc_instance,adc_get_status(&adc_instance));
			adc_start_conversion(&adc_instance);
		delay_ms(500);
	}
//! [inf_loop]
//! [main]
}
