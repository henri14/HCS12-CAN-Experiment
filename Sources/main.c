
/******************************************************************************
												   Copyright (c) Freescale 2011
File Name    : $RCSfile: main.c,v $

Current Revision :	$Revision: 1.0 $

PURPOSE: main program entry.                       
                                                                          
                                                                       
DESCRIPTION:  function main() providing initial program entry.
              function Delay() - simple software delay
              function Wait1ms() - 1mS software delay
              function WaitNms() - N ms software delay
              function ConfigurePorts() - provides I/O set up
                                                         
                                                                          
UPDATE HISTORY                                                            
REV  AUTHOR    DATE        DESCRIPTION OF CHANGE                          
---  ------    --------    ---------------------                          
1.0  r28318    09/06/09    - initial coding
1.1  ra9229    08/23/11    - updated for the S12G

     *******************************************************************
     * File created by: Freescale East Kilbride MSG Applications Group *
     *******************************************************************

                                                                          
******************************************************************************/
/*===========================================================================*/
/* Freescale reserves the right to make changes without further notice to any*/
/* product herein to improve reliability, function, or design. Freescale does*/
/* not assume any  liability arising  out  of the  application or use of any */
/* product,  circuit, or software described herein;  neither  does it convey */
/* any license under its patent rights  nor the  rights of others.  Freescale*/
/* products are not designed, intended,  or authorized for use as components */
/* in  systems  intended  for  surgical  implant  into  the  body, or  other */
/* applications intended to support life, or  for any  other application  in */
/* which the failure of the Freescale product  could create a situation where*/
/* personal injury or death may occur. Should Buyer purchase or use Freescale*/
/* products for any such intended  or unauthorized  application, Buyer shall */
/* indemnify and  hold  Freescale  and its officers, employees, subsidiaries,*/
/* affiliates,  and distributors harmless against all claims costs, damages, */
/* and expenses, and reasonable  attorney  fees arising  out of, directly or */
/* indirectly,  any claim of personal injury  or death  associated with such */
/* unintended or unauthorized use, even if such claim alleges that  Freescale*/
/* was negligent regarding the  design  or manufacture of the part. Freescale*/
/* and the Freescale logo* are registered trademarks of Freescale Ltd.       */
/*****************************************************************************/

/************************* Include Files *************************************/
/*general includes */
#include <hidef.h>  /* also includes boolean definitions in stdtypes.h     */
#include <mc9s12g128.h>     /* derivative information */

/************************* typedefs ******************************************/

/************************* #defines ******************************************/

/************************* Constants *****************************************/

/************************* Global Variables **********************************/
#pragma DATA_SEG DEFAULT

//static byte SerFlag;                   /* Flags for serial communication */
                                       /* Bits: 0 - OverRun error */
                                       /*       1 - Unused */
                                       /*       2 - Unused */
                                       /*       3 - Char in RX buffer */
                                       /*       4 - Full TX buffer */
                                       /*       5 - Running int from TX */
                                       /*       6 - Full RX buffer */
                                       /*       7 - Unused */
//static byte ErrFlag;                   /* Error flags mirror of SerFlag */

/************************* function prototypes *******************************/
#pragma CODE_SEG DEFAULT
/************************* Functions *****************************************/
#pragma CODE_SEG DEFAULT

/* Helper macros to set registers, taken from Processor Expert */
/* Whole peripheral register access macros */
#define setReg8(RegName, val)                                    (RegName = (byte)(val))
#define getReg8(RegName)                                         (RegName)

/*******************************************************************************************************************************
* Name : Delay
* Description : Create a delay for specific time 
* In parameters : time: Time to wait
* Out parameters : None
* Return value : void
********************************************************************************************************************************/
void delay(long int time)		
{
  while(time>>0){--time;}
}

/*
** ===================================================================
**     Method      :  SPI_Init
**
**     Description :
**         Initializes the Serial Peripheral Interface to the
**         CAN System Basis Chip MC33903
** ===================================================================
*/
void SPI_SBC_Init(void)
{
  /* Connected to SPI 1 which uses PJ0:MISO, PJ1:MOSI, PJ2:SCLK, PJ3:SS
  ** MC33903 datasheet
  ** MOSI and MISO data changed at SCLK rising edge
  ** and sampled at falling edge. Msb first.
  ** Use CPHA=1
  */
  /* SPI1CR1: SPIE=0,SPE=0,SPTIE=0,MSTR=0,CPOL=0,CPHA=0,SSOE=0,LSBFE=0 */
  setReg8(SPI1CR1, 0x00U);  /* Disable SPI 1 */

  /* 16 bit transfer (XFRW=1) */
  /* MODe Fault Enabled so that /SS pin is used */
  /* SPI1CR2: ??=0,XFRW=1,??=0,MODFEN=1,BIDIROE=0,??=0,SPISWAI=0,SPC0=0 */
  SPI1CR2 = 0x50U;                     /* Set control register 2 */

  /* Set baud rate to 500kHz */
  /* SPI1BR: ??=0,SPPR2=1,SPPR1=0,SPPR0=0,??=0,SPR2=0,SPR1=1,SPR0=0 */
  SPI1BR = 0x42U;                      /* Set the baud rate register */
  
  /* Enable SPI 1 as master */
  /* SPI1CR1: SPIE=0,SPE=1,SPTIE=0,MSTR=1,CPOL=0,CPHA=1,SSOE=1,LSBFE=0 */
  setReg8(SPI1CR1, 0x56U);
  
  /* Now initialise the SBC chip for CAN operation */
  

  while(!SPI1SR_SPTEF);
  delay(300);
    
  SPI1DRH = 0xDF;       /* Read Vreg register H                         */
  SPI1DRL = 0x80;
  
  while(!SPI1SR_SPTEF); /* Wait till transmit data register is empty    */ 
  delay(300);
    
  /* Disable SPI watchdog refresh command 0x5A00. */
  SPI1DRH = 0x5A;       /* Enter in "Normal Mode                        */
  SPI1DRL = 0x00;  

  while(!SPI1SR_SPTEF); /* Wait till transmit data register is empty    */
  delay(300);  
 
  SPI1DRH = 0x5E;       /* Enable 5V-CAN and Vaux                       */
  SPI1DRL = 0x90;
  
  while(!SPI1SR_SPTEF); /* Wait till transmit data register is empty    */
  delay(300); 
  
  SPI1DRH = 0x60;       /* Set CAN in Tx-Rx mode, fast slew rate        */
  SPI1DRL = 0xC0;
 
}


/*
** ===================================================================
**     Method      :  SPI_CAN_Status
**
**     Description :
**         Read CAN Status from MC33903
** ===================================================================
*/
int SPI_CAN_Status(void)
{
  /* Connected to SPI 1 which uses PJ0:MISO, PJ1:MOSI, PJ2:SCLK, PJ3:SS
  */
  int canStatus = 0;

  (void)SPI1SR;                        /* Read the status register */
  SPI1DR = 0xE100;                     /* SPI commands to get CAN flags status: MOSI 0x E1 00 */
  while ((SPI1SR & 0x80) == 0);        /* Wait for transfer to complete */
  canStatus = SPI1DR;

  SPI1DR = 0xE180;                     /* SPI commands to get CAN flags status: MOSI 0x E1 80 */
  while ((SPI1SR & 0x80) == 0);        /* Wait for transfer to complete */
  canStatus = SPI1DR;

  (void)SPI1SR;                        /* Read the status register */
  SPI1DR = 0x2180;                     /* SPI commands to get CAN real time status: MOSI 0x 21 80 */
  while ((SPI1SR & 0x80) == 0);        /* Wait for transfer to complete */
  canStatus = SPI1DR;

  (void)SPI1SR;                        /* Read the status register */
  SPI1DR = 0xDF00;                     /* SPI commands to get Regulator status: MOSI 0x DF 00 */
  while ((SPI1SR & 0x80) == 0);        /* Wait for transfer to complete */
  canStatus = SPI1DR;

  (void)SPI1SR;                        /* Read the status register */
  SPI1DR = 0xDF80;                     /* SPI commands to get Regulator status: MOSI 0x DF 80 */
  while ((SPI1SR & 0x80) == 0);        /* Wait for transfer to complete */
  canStatus = SPI1DR;

  return canStatus;
  
}



/******************************************************************************
Function Name  : main
Engineer       : r28318	
Date           : 09/06/09
Parameters     : NONE
Returns        : NONE
Notes          : main routine called by Startup.c. 
******************************************************************************/


void main(void)
{
    byte txbuffer;
    int canStatus = 0;
    
    ECLKCTL_NECLK = 0;                  /* Enable ECLK = Bus Clock */                   

    /* initialise the system clock PEE - 64MHz VCO, 32MHz Bus CLK, from 8MHz Crystal */

    CPMUSYNR = 0x47;                    /* SYNDIV = 7, VCO frequency 48 - 64 MHz */       

    CPMUREFDIV = 0x80;                  /* REFDIV = 0 REFCLK frequency 6 - 12 MHz */     

    CPMUPOSTDIV = 0x01;                 /* POSTDIV = 1 */

    CPMUCLKS_PLLSEL = 1;                /* PLLSEL = 1 */

    while (!CPMUCLKS_PLLSEL);           /* Verify CPMUCLKS configuration */

    CPMUOSC_OSCE = 1;                   /* Enable Oscillator OSCE = 1 */

    while(!CPMUFLG_UPOSC);              /* wait for OSC to stabilise */

    while(!CPMUFLG_LOCK);               /* wait for PLL to lock */               
 
    /* LEDs */
  	/* Henri use ports 0-3 instead of 4-7 */
    /* PTT = 0xF0; */                         /* Initialise PORT T all high; turn off LEDs */
    /* DDRT = 0xF0;  */                       /* PORTT 7:4 as outputs */
    PTT = 0x0F;                         /* Initialise PORT T all high; turn off LEDs */
    DDRT = 0x0F;                        /* PORTT 3:0 as outputs */

    /* Initialise SPI to CAN chip */
    SPI_SBC_Init();
    
    /* Push Buttons */    
  	/* Henri use ports 0-3 instead of 4-7 */
  	/* PER1AD = 0xF0; */			          	/* Enable Port PAD Pulls */
  	PER1AD = 0x0F;			          	/* Enable Port PAD Pulls */
  	PPS1AD = 0x00;			            /* Eanble as Pull-Ups */
  	/* Henri use ports 0-3 instead of 4-7 */
  	/* ATDDIEN = 0x00F0;	 */			    /* Enable PAD[7:4] as inputs */
  	ATDDIEN = 0x000F;				    /* Enable PAD[3:0] as inputs */

    /* Initialise the CAN */ 
    CANCTL1_CANE = 1;                   /* enable CAN module */ 
    CANCTL0 = 0x01;                     /* enter init mode */
    while(!(CANCTL1_INITAK));	        /* wait for init mode */

    /*CANCTL1 = 0xA0;	  */                  /* enable CAN module, Loopback Mode, Ext OSC */
    CANCTL1 = 0x80;	                    /* enable CAN module, Ext OSC */
    CANBTR0 = 0xC3;	                    /* sync jump - 4 Tq clocks, prescalar = 3 */
    CANBTR1 = 0x3A;	                    /* Tseg = 3, Tseg1 = 10, 1 sample per bit */
    CANIDAC = 0x10;                   	/* four 16-bit filters */

    CANIDAR0 = 0x20;                    /* Filter 0, ID=0x100 Standard Identifier */
    CANIDMR0 = 0x00;
    CANIDAR1 = 0x00;
    CANIDMR1 = 0x07;                    /* AM[2:0] = 7 to receive standard identifiers */

    CANIDAR2 = 0x00;                    /* Filter 1, ID=0x0000 */
    CANIDMR2 = 0x00;
    CANIDAR3 = 0x00;
    CANIDMR3 = 0x07;                    /* AM[2:0] = 7 to receive standard identifiers */

    CANIDAR4 = 0x00;	                /* Filter 2, ID=0x0000 */
    CANIDMR4 = 0x00;
    CANIDAR5 = 0x00;
    CANIDMR5 = 0x07;                    /* AM[2:0] = 7 to receive standard identifiers */

    CANIDAR6 = 0x00;                  	/* Filter 3, ID=0x0000 */
    CANIDMR6 = 0x00;
    CANIDAR7 = 0x00;
    CANIDMR7 = 0x07;                    /* AM[2:0] = 7 to receive standard identifiers */

    CANCTL0 = 0x00;	                    /* exit init mode */
    while(CANCTL1_INITAK);			    /* wait until module exits init mode */

    while(!(CANCTL0_SYNCH));		    /* wait for CAN module to synch */

    CANRFLG = 0xC3;					    /* reset Rx flags */

    canStatus = SPI_CAN_Status();
    
    for(;;)
    {
        while (!CANTFLG);               /* Wait for empty Tx Buffer */
        CANTBSEL = CANTFLG;	            /* Select the empty Tx Buffer */
        txbuffer = CANTBSEL;            /* Save the empty buffer */

        CANTXIDR0 = 0x20;               /* load message id value to ID regs */
        CANTXIDR1 = 0x00;
        CANTXIDR2 = 0x00;
        CANTXIDR3 = 0x00;

        while(PT1AD == 0xFF);
        
        CANTXDSR0 = PT1AD;	            /* load data to send */

        CANTXDLR = 0x01;			    /* set data length */
        CANTXTBPR = 0x80;		        /* set data buffer priority */

        CANTFLG = txbuffer;             /* start transmission */

        while((CANTFLG & txbuffer) != txbuffer)   /* wait for Tx to complete */
        {
          canStatus = SPI_CAN_Status();
        }

        if(CANRFLG_RXF)                 /* has a message been received ? */
        {
            PTT = CANRXDSR0;            /* Display transmitted PORTB on LEDs */
            CANRFLG_RXF = 1;            /* Clear RXF */
        }
    }

}

#pragma CODE_SEG DEFAULT