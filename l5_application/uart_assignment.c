#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "gpio.h"
#include "lpc40xx.h"

#include "board_io.h"
#include "delay.h"
#include "uart.h"
#include "uart_printf.h"

#include "uart_lab.h"
#include <stdio.h>
QueueHandle_t uart_data;

void uart2_receive_data_handler() {
  char data;
  data = LPC_UART2->RBR;

  xQueueSendFromISR(uart_data, &data, NULL);

  // read data into freertos queue
}

void send_data() {

  while (1) {
    uart_printf__polled(UART__0, "Sending Data\n");
    uart_lab__polled_put(UARTL__2, 'C');
    uart_printf__polled(UART__0, "Data Sent\n");
    vTaskDelay(100);
  }
}

void receive_data() {

  char data = 'E';
  while (1) {
    uart_lab__polled_get(UARTL__2, &data);

    uart_printf__polled(UART__0, "Recieved: %c\n", data);

    vTaskDelay(100);
  }
}

void print_data() {
  char data;
  while (1) {
    uart_printf__polled(UART__0, "Waiting For Data from Queue\n");
    if (xQueueReceive(uart_data, &data, portMAX_DELAY)) {
      uart_printf__polled(UART__0, "Received from interrupt: %c\n", data);
    }

    vTaskDelay(100);
  }
}

// part3
void board_2_receiver_task(void *p) {
  char number_as_string[16] = {0};
  int counter = 0;

  while (true) {
    char byte = 0;
    if (xQueueReceive(uart_data, &byte, portMAX_DELAY)) {
    }
    printf("Received: %c\n", byte);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      printf("Received this number from the other board: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      number_as_string[counter] = byte;
      if (counter < 16) {
        counter++;
      }
      // Hint: Use counter as an index, and increment it as long as we do not reach max value of 16
    }
  }
}

int main6() {
  uart_data = xQueueCreate(16, sizeof(char));
  uart0_init();
  uart_printf__polled(UART__0, "Starting UART interrupt receive\n");

  uart_lab__init(UARTL__2, clock__get_peripheral_clock_hz(), 115200);
  uart_pinsel_init(UARTL__2);

  // part 1
  // xTaskCreate(send_data, "send_data", (512U * 4) / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(receive_data, "recieve_data", (512U * 4) / sizeof(void *), NULL, 1, NULL);

  // interrupt for part 2 and 3
  uart_lab_attach_interrupt(UARTL__2, receive_intr, uart2_receive_data_handler);

  // part2
  // xTaskCreate(send_data, "send_data", (512U * 4) / sizeof(void *), NULL, 1, NULL);
  // xTaskCreate(print_data, "recieve_data", (512U * 4) / sizeof(void *), NULL, 1, NULL);

  // part 3
  xTaskCreate(board_2_receiver_task, "receiveing_board", (512U * 4) / sizeof(void *), NULL, 1, NULL);
  vTaskStartScheduler();
}