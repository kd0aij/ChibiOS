/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

#define MAPLE
#if defined(MAPLE)
#define TXPORT GPIOA
#define TXPIN 12
#define LEDPORT GPIOB
#define LEDPIN 1

#elif defined(MRO)
#define TXPORT GPIOA
#define TXPIN 12
#define LEDPORT GPIOA
#define LEDPIN 4
#endif

#define CONFIG_LED  palSetPadMode(LEDPORT, LEDPIN, PAL_MODE_OUTPUT_PUSHPULL)
#define LED_ON palSetPad(LEDPORT, LEDPIN)
#define LED_OFF palClearPad(LEDPORT, LEDPIN)
#define LED_TOGGLE palTogglePad(LEDPORT, LEDPIN)

#define CONFIG_TX  palSetPadMode(TXPORT, TXPIN, PAL_MODE_OUTPUT_PUSHPULL)
#define TX_HIGH palSetPad(TXPORT, TXPIN)
#define TX_LOW palClearPad(TXPORT, TXPIN)
#define TX_TOGGLE palTogglePad(TXPORT, TXPIN)


/*
 * Internal loopback mode, 500KBaud, automatic wakeup, automatic recover
 * from abort mode.
 * See section 22.7.7 on the STM32 reference manual.
 */
static const CANConfig cancfg = {
  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
    CAN_BTR_LBKM | CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
//    CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
  CAN_BTR_TS1(8) | CAN_BTR_BRP(6)
};

#if 1
/*
 * Receiver thread.
 */
static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx, p) {
  event_listener_t el;
  CANRxFrame rxmsg;

  (void)p;
  chRegSetThreadName("receiver");
  chEvtRegister(&CAND1.rxfull_event, &el, 0);
  while (true) {
    if (chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(100)) == 0)
      continue;
    while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      LED_OFF;
    }
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
}

/*
 * Transmitter thread.
 */
static THD_WORKING_AREA(can_tx_wa, 256);
static THD_FUNCTION(can_tx, p) {
  CANTxFrame txmsg;

  (void)p;
  chRegSetThreadName("transmitter");
  txmsg.IDE = CAN_IDE_EXT;
  txmsg.EID = 0x01234567;
  txmsg.RTR = CAN_RTR_DATA;
  txmsg.DLC = 8;
  txmsg.data32[0] = 0x55AA55AA;
  txmsg.data32[1] = 0x00FF00FF;

//  AFIO->MAPR &= ~AFIO_MAPR_CAN_REMAP_REMAP3_Msk;    // clear the CAN remap bits
//  rccEnableCAN1(1);
//  rccEnableAPB2((1<<2), 1);
  // CAN RX
  palSetPadMode(GPIOA, 11, PAL_MODE_INPUT);

  while (true) {
    LED_ON;
    CONFIG_TX;
    canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, MS2ST(100));
    chThdSleepMilliseconds(500);
  }
}
#endif

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  CONFIG_LED;
  CONFIG_TX;

  /*
   * Activates the CAN driver 1.
   */
  canStart(&CAND1, &cancfg);

  /*
   * Starting the transmitter and receiver threads.
   */
  chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO + 7, can_rx, NULL);
  chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO + 7, can_tx, NULL);


  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (true) {
//    TX_TOGGLE;
    LED_TOGGLE;
    chThdSleepMilliseconds(500);
  }
  return 0;
}
