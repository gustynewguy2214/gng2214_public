/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR69xx Demo - eUSCI_A0 UART echo at 9600 baud using BRCLK = 8MHz
//
//  Description: This demo echoes back characters received via a PC serial port.
//  SMCLK/ DCO is used as a clock source and the device is put in LPM3
//  The auto-clock enable feature is used by the eUSCI and SMCLK is turned off
//  when the UART is idle and turned on when a receive edge is detected.
//  Note that level shifter hardware is needed to shift between RS232 and MSP
//  voltage levels.
//
//  The example code shows proper initialization of registers
//  and interrupts to receive and transmit data.
//  To test code in LPM3, disconnect the debugger.
//
//  ACLK = VLO, MCLK =  DCO = SMCLK = 8MHz
//
//                MSP430FR6989
//             -----------------
//       RST -|     P2.0/UCA0TXD|----> PC (echo)
//            |                 |
//            |                 |
//            |     P2.1/UCA0RXD|<---- PC
//            |                 |
//
//   William Goh
//   Texas Instruments Inc.
//   April 2014
//   Built with IAR Embedded Workbench V5.60 & Code Composer Studio V6.0
//******************************************************************************
// modified by: Peter Spevak
//
// last change: April 7th 2020
//
// 04/07/2020 changing the UART to eUSCIA1, as this is the UART for
//            backchannel on virtual COM port to PC
//
//*****************************************************************************
#include <msp430.h>

#include "msp430fr6989.h"
#include "Grlib/grlib/grlib.h"          // Graphics library (grlib)
#include "LcdDriver/lcd_driver.h"       // LCD driver
#include <stdio.h>
#include <string.h>

char read_string[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
static char disp_1[8] =      {' ',' ',' ',' ',' ',' ',' ',' '};
static char disp_2[8] =      {' ',' ',' ',' ',' ',' ',' ',' '};
static char disp_3[8] =      {' ',' ',' ',' ',' ',' ',' ',' '};

volatile unsigned int j = 0;

bool update = false;
bool update_complete = false;

// Initial clock: SMCLK @ 1.048 MHz with oversampling
void Initialize_UART(void){
    // Divert pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);
    // Use SMCLK clock; leave other settings default
    UCA1CTLW0 |= UCSSEL_2;
    // Configure the clock dividers and modulators
    // UCBR=6, UCBRF=13, UCBRS=0x22, UCOS16=1 (oversampling)
    UCA1BRW = 6;
    UCA1MCTLW = UCBRS5|UCBRS1|UCBRF3|UCBRF2|UCBRF0|UCOS16;
    // Exit the reset state (so transmission/reception can begin)
    UCA1CTLW0 &= ~UCSWRST;
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;                 // Stop Watchdog

  // Configure GPIO
  //P2SEL0 |= BIT0 | BIT1;                    // USCI_A0 UART operation         // 04/07/2020
  //P2SEL1 &= ~(BIT0 | BIT1);                                                   // 04/07/2020
  P3SEL0 |= BIT4 + BIT5;                    // eUSCI_A1 UART                    // 04/07/2020
  P3SEL1 &= ~(BIT4 + BIT5);                 // eUSCI_A1 UART                    // 04/07/2020

  // Disable the GPIO power-on default high-impedance mode to activate
  // previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;


  //Initialize_UART();

  // Configure SMCLK to 8 MHz (used as SPI clock)
  CSCTL0 = CSKEY;                  // Unlock CS registers
      CSCTL3 &= ~(BIT4|BIT5|BIT6);    // DIVS=0
      CSCTL0_H = 0;                   // Relock the CS registers


      /*
  // Startup clock system with max DCO setting ~8MHz
  CSCTL0_H = CSKEY >> 8;                    // Unlock clock registers
  CSCTL1 = DCOFSEL_3 | DCORSEL;             // Set DCO to 8MHz
  CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
  CSCTL0_H = 0;                             // Lock CS registers

  // Configure USCI_A0 for UART mode
  UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset             // 04/07/2020
  UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
  // Baud Rate calculation
  // 8000000/(16*9600) = 52.083
  // Fractional portion = 0.083
  // User's Guide Table 21-4: UCBRSx = 0x04
  // UCBRFx = int ( (52.083-52)*16) = 1
  UCA0BR0 = 52;                             // 8000000/16/9600
  UCA0BR1 = 0x00;
  UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
  UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
*/


  // Configure USCI_A1 for UART mode
  UCA1CTLW0 = UCSWRST;                      // Put eUSCI in reset               // 04/07/2020
  UCA1CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK                      // 04/07/2020
  // Baud Rate calculation
  // 8000000/(16*9600) = 52.083
  // Fractional portion = 0.083
  // User's Guide Table 21-4: UCBRSx = 0x04
  // UCBRFx = int ( (52.083-52)*16) = 1
  UCA1BR0 = 52;                             // 8000000/16/9600                  // 04/07/2020
  UCA1BR1 = 0x00;                                                               // 04/07/2020
  UCA1MCTLW |= UCOS16 | UCBRF_1 | 0x4900;                                       // 04/07/2020
  UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI                 // 04/07/2020
  UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt      // 04/07/2020

  _enable_interrupts();

      Graphics_Context g_sContext;        // Declare a graphic library context
      Crystalfontz128x128_Init();         // Initialize the display

      // Set the screen orientation
      Crystalfontz128x128_SetOrientation(0);

      // Initialize the context
      Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);

      // Set background and foreground colors
      Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
      Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);

      // Set the default font for strings
      GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

      // Clear the screen
      Graphics_clearDisplay(&g_sContext);
      ////////////////////////////////////////////////////////////////////////////////////////////

      Graphics_drawStringCentered(&g_sContext, "Info:", AUTO_STRING_LENGTH, 64, 30, OPAQUE_TEXT);

      char old[6] = {65,'2','\0','\0','\0','\0'};

      while(1){

          Graphics_drawStringCentered(&g_sContext,read_string, AUTO_STRING_LENGTH, 32, 55, OPAQUE_TEXT);

          if(update){

              /*
              tRectangle rectangle1 = {0,45,128,128}; //x1, y1, x2, y2

              //Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_PINK);
             // Graphics_drawRectangle(&g_sContext,&rectangle1);

              Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
              Graphics_fillRectangle(&g_sContext,&rectangle1);

              Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);

              Graphics_drawStringCentered(&g_sContext,read_string, AUTO_STRING_LENGTH, 32, 55, OPAQUE_TEXT);
*/

              update = false;



              /*
              char error_msg[30] = {0};
              char error = ' ';

              if(read_string[0] == '*'){
                  int z = 0;

                  switch(read_string[1]){
                      case '1':
                          //Graphics_drawStringCentered(&g_sContext,read_string, AUTO_STRING_LENGTH, 32, 55, OPAQUE_TEXT);
                         // break;

                          for(z = 1; z < 8; z++){
                              //if(z-1 != 6){
                                  disp_1[z-1] = read_string[z];
                              //}
                          }
                          break;


                      case '2':
                          //Graphics_drawStringCentered(&g_sContext,read_string, AUTO_STRING_LENGTH, 102, 75, OPAQUE_TEXT);
                          //break;


                          for(z = 1; z < 8; z++){
                              //if(z-1 != 6){
                                  disp_2[z-1] = read_string[z];
                              //}
                          }
                          break;


                      case '3':
                        for(z = 1; z < 8; z++){
                            //if(z-1 != 6){
                                disp_3[z-1] = read_string[z];
                            //}
                        }
                        break;
                      default:
                          //error = read_string[1];

                          //error_msg[10] = error;
                          break;
                  }
              }
              else{
                  error_msg[10] = ' ';
              }


              Graphics_drawStringCentered(&g_sContext,disp_1, AUTO_STRING_LENGTH, 0, 55, OPAQUE_TEXT);
              Graphics_drawStringCentered(&g_sContext,disp_2, AUTO_STRING_LENGTH, 0, 75, OPAQUE_TEXT);
              Graphics_drawStringCentered(&g_sContext,disp_3, AUTO_STRING_LENGTH, 0, 95, OPAQUE_TEXT);
              Graphics_drawStringCentered(&g_sContext,error_msg, AUTO_STRING_LENGTH, 0, 105, OPAQUE_TEXT);
              Graphics_drawStringCentered(&g_sContext,read_string, AUTO_STRING_LENGTH, 0, 115, OPAQUE_TEXT);

              update = false;

              if(update_complete){
                  //Clear the array
                volatile unsigned int k = 0;
                for(k = 0; k < (sizeof(read_string)/sizeof(char)); k++){
                    read_string[k] = '\0';
                }
                update_complete = false;
              }
              */
          }

      }

}

/*
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      while(!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = UCA0RXBUF;
      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}
*/

volatile unsigned int ch = NULL;

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)            // 04/07/2020
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      while(!(UCA1IFG&UCTXIFG));
      //UCA1TXBUF = UCA1RXBUF;
      //ch = UCA1RXBUF;


        read_string[j] = UCA1RXBUF; //https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/72124?Receive-strings-in-UART

        /*
      if(ch != '\n'){
          //read_string[j] = ch;



      }
      else{
          j = 0;
          update_complete = true;
      }
*/

      j++;
                if (j > 7){

                    j = 0;

                    int i = 0;
                    for(i = 0; i < 8; i++){
                        read_string[i] = ' ';
                    }
                    //i = 0;
                    //UCA1TXBUF = string1[i];
                    //i++;
                }

      update = true;

      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

