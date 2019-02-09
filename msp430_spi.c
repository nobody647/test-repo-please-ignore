/* msp430_spi.c
 * Library for performing SPI I/O on a wide range of MSP430 chips.
 *
 * Serial interfaces supported:
 * 1. USI - developed on MSP430G2231
 * 2. USCI_A - developed on MSP430G2553
 * 3. USCI_B - developed on MSP430G2553
 * 4. USCI_A F5xxx - developed on MSP430F5172, added F5529
 * 5. USCI_B F5xxx - developed on MSP430F5172, added F5529
 * 6. USCI_A F4xxx - developed on MSP430F4133
 *
 * Copyright (c) 2013, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <msp430.h>
#include "driverlib.h"
#include "inc/hw_memmap.h"

/*
 * Function that will initialize the EUSCI_A0_SPI module
 * of the controller as the master in the SPI communication bus
 *
 * TODO After checking if the MCP2515 needs to have all of the messages sent
 * TODO while the CS is low, now switching between each part of the message
 * TODO the mode might need to be changed to three wire mode
*/
void spi_init() {
//
//    __bis_SR_register(SCG0);                // disable FLL
//    CSCTL3 |= SELREF__REFOCLK;              // Set REFO as FLL reference source
//    CSCTL0 = 0;                             // clear DCO and MOD registers
//    CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
//    CSCTL1 |= DCORSEL_3;                    // Set DCO = 8MHz
//    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
//    __delay_cycles(3);
//    __bic_SR_register(SCG0);                // enable FLL
//    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
//
//
//    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
//                                            // default DCODIV as MCLK and SMCLK source
//

    // Configure SPI pins
    // Configure Pins for UCB0CLK, UCB0TXD/UCB0SIMO and UCB0RXD/UCB0SOMI
    /*
    * Select Port 5
    * Set Pin 0, Pin 1, Pin 2 and Pin 3 to input with function.
    * P5.0 - CS
    * P1.2 - Clock Signal
    * P1.0 - MOSI
    * P1.1 - MISO
    */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2,
        GPIO_PRIMARY_MODULE_FUNCTION
    );



    /*
     * Check that by this point that the GPIO port that has P5.0
     * is set as an output. If it is not then remove P5.0 from the above statement
     * and uncomment the two following lines
     */

    /*
     * Set the P5.0 Pin as output to be used for the CS pin
     * and set the pin to low as the scheme that is being used is ActiveHigh
     */
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0);               //set P5.0 as output
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);           // set the value of P5.0 to LOW



    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PMM_unlockLPM5();


    //    Set all of the params for the SPI module
    EUSCI_A_SPI_initMasterParam SPIparams;

    //    Source of the clock for the SPI communication bus
    SPIparams.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
    //    Frequency of the source selected above
    SPIparams.clockSourceFrequency = CS_getSMCLK();
    //    Clock speed that the SPI bus will operate at
    SPIparams.desiredSpiClock = 500000;
    //    Select the Endian-ness of the communications that will take place on the bus
    //    whether the data will be sent LSB or MSB
    SPIparams.msbFirst = EUSCI_A_SPI_MSB_FIRST;
    //    Select the clock phase
    SPIparams.clockPhase = EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
    //    Select the clock polarity whcih determines if data is read when the CS line is high or low
    //    By default data is read when the CS line is low
    SPIparams.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    //    Select the mode that the BUS will operate in
    //    Active low means that data is readwhen the CS line is at a low value
//    SPIparams.spiMode = EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW;
    //    Uncomment this line if you want to use three wire mode
    SPIparams.spiMode = EUSCI_A_SPI_3PIN;


    //    Initialize the SPI module with the settings from above
    EUSCI_A_SPI_initMaster(EUSCI_A0_BASE, &SPIparams);

    /*
     * Master init handles the setting up of the module for three wire operation
     * so if you are looking to use four-wire mode then you have to use the line below to set up
     */
    //    Comment out the line above if using three wire mode and swithcing the CS line manually
//    EUSCI_A_SPI_select4PinFunctionality(EUSCI_A0_BASE, EUSCI_A_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVEs);



    //    Actually enable the SPI block for operation
    EUSCI_A_SPI_enable(EUSCI_A0_BASE);



    //    Clear any outstanding interrupts from the SPI module
    EUSCI_A_SPI_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);


    // Enable USCI_B0 RX interrupt
    EUSCI_A_SPI_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);




}

/*
    Function that will send a block of data to a slave and wait for
    the ACK that the slave received it. Upon receiving the response the function will
    terminate and return the byte returned from the slave
*/
uint8_t spi_transfer(uint8_t message) {



    //    Transmit Data to slave
    EUSCI_A_SPI_transmitData(EUSCI_A0_BASE, message);

    //    wait for the tranmission to complete
    while( EUSCI_A_SPI_isBusy(EUSCI_A0_BASE) == EUSCI_A_SPI_BUSY ) {};


    // Uncomment the code below to handle respones from the SPI module
    //    Wait for there to be a message returned from the target device
    while( !EUSCI_A_SPI_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT) );

    //    Return the message that was sent back from the slave
    uint8_t returnedMessage = EUSCI_A_SPI_receiveData(EUSCI_A0_BASE);

    //    Clear the received interrupt flag so that it doesn't get read again
    EUSCI_A_SPI_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);

    //    return the packet that was the response of the slave device
    return returnedMessage;

    return 0x00;


}
