/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <uart_handler.h>
#include <dk_buttons_and_leds.h>

#define SLEEP_TIME_MS 	5000
#define TX_BUF_SIZE 	1000

static uint8_t app_uart_tx_buf[TX_BUF_SIZE];
static uint8_t app_uart_rx_buf[8192];
static int app_uart_rx_buf_cnt = 0;

static void on_app_uart_event(struct app_uart_evt_t *evt)
{
	switch(evt->type) {
		// Data received over the UART interface. 
		// NOTE: The UART data buffers are only guaranteed to be retained until this function ends
		//       If the data can not be processed immediately, they should be copied to a different buffer
		case APP_UART_EVT_RX:
			// Simply echo any incoming data back to the sender
			//app_uart_send(evt->data.rx.bytes, evt->data.rx.length, K_FOREVER); 
			printk("RX %i-", evt->data.rx.length);
			memcpy(&app_uart_rx_buf[app_uart_rx_buf_cnt], evt->data.rx.bytes, evt->data.rx.length);
			app_uart_rx_buf_cnt += evt->data.rx.length;
			break;

		// A UART error ocurred, such as a break or frame error condition
		case APP_UART_EVT_ERROR:
			printk("UART error: Reason %i\n", evt->data.error.reason);
			break;

		// The UART event queue has overflowed. If this happens consider increasing the UART_EVENT_QUEUE_SIZE (will increase RAM usage),
		// or increase the UART_RX_TIMEOUT_US parameter to avoid a lot of small RX packets filling up the event queue
		case APP_UART_EVT_QUEUE_OVERFLOW:
			printk("UART library error: Event queue overflow!\n");
			break;
	}
}

#define BTN1_MSK (1 << 0)
#define BTN2_MSK (1 << 1)
#define BTN3_MSK (1 << 2)
#define BTN4_MSK (1 << 3)
int button_pressed = 0;
static void on_button_press(struct k_work *work)
{
	int length;
	switch(button_pressed) {
		case 1:
			length = TX_BUF_SIZE;
			printk("Sending %i byte buffer\n", length);
			app_uart_send(app_uart_tx_buf, length, K_FOREVER);
			break;
		case 2:
			length = TX_BUF_SIZE / 10;
			printk("Sending %i byte buffer\n", length);
			app_uart_send(app_uart_tx_buf, length, K_FOREVER);
			break;
		case 3:
			length = TX_BUF_SIZE / 100;
			printk("Sending %i byte buffer\n", length);
			app_uart_send(app_uart_tx_buf, length, K_FOREVER);
			break;
		case 4:
			length = 1;
			printk("Sending %i byte buffer\n", length);
			app_uart_send(app_uart_tx_buf, length, K_FOREVER);
			break;
	}
}

K_WORK_DEFINE(work_on_button_press, on_button_press);

void dk_btn_handler(uint32_t button_state, uint32_t has_changed)
{	
	int length;
	if((button_state & BTN1_MSK) && (has_changed & BTN1_MSK)) {
		button_pressed = 1;
		k_work_submit(&work_on_button_press);
	}
	if((button_state & BTN2_MSK) && (has_changed & BTN2_MSK)) {
		button_pressed = 2;
		k_work_submit(&work_on_button_press);
	}
	if((button_state & BTN3_MSK) && (has_changed & BTN3_MSK)) {
		button_pressed = 3;
		k_work_submit(&work_on_button_press);
	}
	if((button_state & BTN4_MSK) && (has_changed & BTN4_MSK)) {
		button_pressed = 4;
		k_work_submit(&work_on_button_press);
	}
}

void main(void)
{
	int err;

	printk("App Uart 2-kit test sample started\n");

	dk_buttons_init(dk_btn_handler);
	
	err = app_uart_init(on_app_uart_event);
	if(err != 0) {
		printk("app_uart_init failed: %i\n", err);
		return;
	}

	for(int i = 0; i < TX_BUF_SIZE; i++) {
		app_uart_tx_buf[i] = (i % 25) + 'a';
	}

	while(1) {
		k_msleep(SLEEP_TIME_MS);
		if(app_uart_rx_buf_cnt > 0) {
			printk("\n%i bytes received\n", app_uart_rx_buf_cnt);
			app_uart_rx_buf_cnt = 0;
		}

		static uint8_t my_data[] = {1,2,3,4,5};
		app_uart_send(my_data, sizeof(my_data), K_FOREVER);
	}
}
