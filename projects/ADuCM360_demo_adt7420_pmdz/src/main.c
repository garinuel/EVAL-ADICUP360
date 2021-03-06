//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------
/***************************** Include Files **********************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include <ADuCM360.h>
#include <Communication.h>
#include "ADT7420.h"
#include "UrtLib.h"
#include "I2cLib.h"

// ----------------------------------------------------------------------------
//
// Standalone $(shortChipFamily) empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/******************************************************************************/
/************************** Variable Definitions ******************************/
/******************************************************************************/
uint16_t volatile ui16I2cStatus = 0;

/******************************************************************************/
/************************* Functions Definitions ******************************/
/******************************************************************************/

/**
  @brief Looks to see if there is anything available in the UART.

  @return none
**/
void UART_Int_Handler (void)
{

   unsigned short  status;
   char c;

   status = UrtIntSta(pADI_UART);                      /* Check UART status */

   if (status & COMIIR_NINT) {
      return;   /* Check if UART is busy */
   }

   switch (status & COMIIR_STA_MSK) {                  /* Check what command to execute */
   case COMIIR_STA_RXBUFFULL:                         /* Check if UART register is available to be read */

      UART_ReadChar(&c);                             /* Read character from UART */

      if( c == 13) {                            /* Check if read character is ENTER */
         uart_cmd = UART_TRUE;                        /* Set flag */
      }

      break;


   case COMIIR_STA_TXBUFEMPTY:                      /* Check if UART register is available to be written */

      if (uart_tcnt) {                              /* Check uart counter */

         uart_tbusy = UART_TRUE;                    /* UART is busy with writing*/

         uart_tcnt--;                               /* Decrease  uart counter */

         UART_WriteChar(uart_tx_buffer[uart_tpos++], UART_WRITE);          /* Write character to UART */

         if (uart_tpos == UART_TX_BUFFER_SIZE) {                         /* Check if TX buffer is full */
            uart_tpos = 0;                      /* Reset buffer counter  */
         }

      } else {

         uart_tbusy = UART_FALSE;                  /* UART is no longer busy with writing */
      }

      break;

   default:
      ;
   }

}


/**
  @brief I2C0 Interrupt Handler

  @return none

**/
void I2C0_Master_Int_Handler(void)
{

   ui16I2cStatus = 0;
   ui16I2cStatus = I2cSta(MASTER);

  if((ui16I2cStatus & I2CMSTA_RXREQ) == I2CMSTA_RXREQ)  // Master Receive IRQ
     {
        uiMasterRxDat[uiMasterRxIndex++] = I2cRx(MASTER);
     }

}

/******************************************************************************/
/******************************** Main Program ********************************/
/******************************************************************************/

int
main(int argc, char* argv[])
{

   // Initialize main program variables
   uint16_t volatile ui16tempResults = 0;
   uint16_t volatile ui16IdStatus = 0;
   float fTempResults = 0.0;

   // Initialize program setup
   I2C_Init();

   UART_Init(B9600, COMLCR_WLS_8BITS);

   ADT7420_Init();


   while (1)
    {

        ui16IdStatus = ADT7420_Read_One_Reg (ID_REG);

        ui16tempResults = ADT7420_Read_Temp();  /* Provides code out of sensor, need to convert this to degrees C */

        fTempResults = ADT7420_Convert_Hex_To_Degrees (ui16tempResults);  /* Converts digital data to degrees C */


        /* Looks to see if the <ENTER KEY> has been pressed, if yes, display data on terminal */
        if (uart_cmd == UART_TRUE) {

        UART_Printf("\r\n The ID register data for the ADT7420 is: 0x%2x",ui16IdStatus);

        UART_Printf("\r\n The ADC temp data of the ADT7420 is: 0x%04x", ui16tempResults);

        UART_Printf("\r\n The temperature is: %f degrees C ", fTempResults);

        UART_Printf("\r\n");

        uart_cmd = UART_FALSE;
        }

    }
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
