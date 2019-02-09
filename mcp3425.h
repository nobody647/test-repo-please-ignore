#include "driverlib.h"
#include <msp430.h>

// each slave's address, can change names if so inclined...
#define slave_25 0x00     /// 1101000

#define slave_28_num1  0x00 // must make one Ad1/0 bit float****
#define slave_28_num2  0x6A
#define slave_28_num3  0x6C

#define SLAVE_ADDRESS 0x48
  //Target frequency for SMCLK in kHz
#define CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ 1000
  //SMCLK/FLLRef Ratio
#define CS_SMCLK_FLLREF_RATIO 30

    int slave_25_data [16] = {};
    int slave_28_data [12] = {};

    int MUX_select = 0;
    int slave_28_num1_channel_index = 0;
    int slave_28_num2_channel_index = 0;
    int slave_28_num3_channel_index = 0;

class adc_lib
{
    public:

    const int slave_config [4] = {0x01, 0x02, 0x007, 0x08}; // first
    int config_index = 0;

    void set_clocks( void );   //  ONLY  FOR  PRACTICE and testing the program with a demo main file.

    void init_i2c_bus ( void );     // will enable i2c and set slaves and stuff

    void set_mux (int&);   // bit wise operations writing to each pin for selected number

    void get_slave_25_info ( int select_pin );
                                            //  to be used in each interrupt to collect the data
    void get_slave_28_info ( int slave_address );
};  // you DO need a semicolon here!


void adc_lib :: set_clocks( void )
{   //Set DCO FLL reference = REFO
    CS_initClockSignal( CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    //Set Ratio and Desired MCLK Frequency  and initialize DCO
    CS_initFLLSettle( CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ, CS_SMCLK_FLLREF_RATIO );

    CS_initClockSignal( CS_ACLK, CS_VLOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal( CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal( CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1 );
}  // taken from driver lib example of masterRXmultiple


void adc_lib :: set_mux( int &s )   // let s be selection number
{
    if( s < 16 )    // pin 1 will be addressed as 0, like in schematic
    {
        if( (s>>3)%2 != (MUX_select>>3)%2 )
            GPIO_toggleOutputOnPin( GPIO_PORT_P7, GPIO_PIN3 );
        if( (s>>2)%2 != (MUX_select>>2)%2 )
            GPIO_toggleOutputOnPin( GPIO_PORT_P7, GPIO_PIN2 );
        if( (s>>1)%2 != (MUX_select>>1)%2 )
            GPIO_toggleOutputOnPin( GPIO_PORT_P7, GPIO_PIN1 );
        if( s%2 != MUX_select%2 )
            GPIO_toggleOutputOnPin( GPIO_PORT_P7, GPIO_PIN0 );
    }
}


void adc_lib :: init_i2c_bus ( void )
{

    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P5,
        GPIO_PIN2 + GPIO_PIN3,
        GPIO_PRIMARY_MODULE_FUNCTION );

    GPIO_setAsOutputPin( GPIO_PORT_P7, GPIO_PIN3 ); // S3 pin
    GPIO_setAsOutputPin( GPIO_PORT_P7, GPIO_PIN2 ); // S2 pin
    GPIO_setAsOutputPin( GPIO_PORT_P7, GPIO_PIN1 ); // S1 pin
    GPIO_setAsOutputPin( GPIO_PORT_P7, GPIO_PIN0 ); // S0 pin

    EUSCI_B_I2C_initMasterParam param = {0};
        param.selectClockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
        param.i2cClk = CS_getSMCLK();
        param.dataRate = EUSCI_B_I2C_SET_DATA_RATE_400KBPS;
        param.byteCounterThreshold = 2;
        param.autoSTOPGeneration = EUSCI_B_I2C_NO_AUTO_STOP;    // MUST CHANGE THIS

    EUSCI_B_I2C_initMaster(EUSCI_B0_BASE, &param);
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, slave_25 );  //Specify slave address
    EUSCI_B_I2C_setMode( EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_MODE  );

    UCB0I2COA0 |= slave_25 | UCOAEN;
    UCB0I2COA1 |= slave_28_num3 | UCOAEN;
    UCB0I2COA2 |= slave_28_num2 | UCOAEN;
    UCB0I2COA3 |= slave_28_num1 | UCOAEN;

    EUSCI_B_I2C_enable(EUSCI_B0_BASE);
    UCB0IE |=  UCTXIE3 | UCNACKIFG;

    EUSCI_B_I2C_masterSendMultiByteStart( EUSCI_B0_BASE, slave_config[config_index++] );
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B0_BASE));//****

    EUSCI_B_I2C_masterSendMultiByteStart( EUSCI_B0_BASE, slave_config[config_index++] );
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B0_BASE));
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, slave_28_num2 );

    EUSCI_B_I2C_masterSendMultiByteStart( EUSCI_B0_BASE, slave_config[config_index++] );
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B0_BASE));
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, slave_28_num3 );

    EUSCI_B_I2C_masterSendMultiByteStart( EUSCI_B0_BASE, slave_config[config_index++] );
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B0_BASE));

    UCB0IE &= ~UCTXIE3;
    UCB0IE |=  UCRXIE0 | UCRXIE1| UCRXIE2 | UCRXIE3; //nack bit IFG already enabled..
}
