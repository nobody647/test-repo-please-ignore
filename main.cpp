#include <msp430.h> 
#include "driverlib.h"
#include "CAN_messages.h"

//test?

/*
 * TODO !!! One thing that you could also look into is
 * the github integration with CCS, there should be a way that
 * we can put a entire project up there and pull it down and it still be a
 * CCS project.
 * Currently when a project is placed in the repo and pulled it is imported as
 * a standard c/c++ project which is a pain to convert back to a CCS project
 *
 * Priority one is finishing this code, then we can worry about fiddling with
 * the repo
 */



// Libraries for the MCP2515 CAN module
#include "msp430_spi.c"
#include "mcp2515.c"




// Libraries for the MCP3428 ADC modules
/*

#include "mcp3425.h"



/*
 * Constants used for the BMS Slave module
 */


/*
 * This will differ from modul to module
 * TODO remove this and make the ID of the module be assigned
 * either by dip-switches or dynmaically, but more likely dipswitches
 *
 * TODO Talk to Peter about how the slave id will be set, either with
 * the dipswitch thing, and if so, what pins will be used for the dipswitch array
 *
 */
#define SLAVE_MODULE_ID 0x00


/*
 * Constants for the voltage, temperature, and current limits
 */

//#define VOLTAGE_LOW_LIMIT 2.7
#define VOLTAGE_LOW_LIMIT 8900
// TODO detemine the ADC value that will corespond to this voltage
// TODO will replace with the voltage level in the future

//#define VOLTAGE_HIGH_LIMIT 4.2
#define VOLTAGE_HIGH_LIMIT 15000
// TODO convert the voltage high limit to ADC value like the low limit

// Max temperature of the cells in celcius

/*
 * TODO There will eventually be two set points, one fro charging and one for discharging, after we get everything else going
 *
 * !!! For the time being just worry about the one temperature !!!
 */
#define TEMPERATURE_LIMIT_DISCHARGING 63







/*
 * globals used for storing the current state of the
 * battery modules including the
 */

/*
 * Module voltages that are being stored awaiting transmission
 */
uint32_t module_voltages[12];




uint32_t module_temperatures[12][5]; //2d arrays make my brain hurt


//For temp
//5 measurements per moudle
//32 modules
//Each slave measures 12 modules, except one which is short





//Flag is set to true if any measurements are outside of the acceptable range. 0 means everything is ok, 1 means voltage error, 2 means temperature
uint32_t module_error_state = 0;

//Which ADC to sample from. First ADC is 1, 2nd is 2, etc.
uint32_t adc_to_sample = 1;

//Which channel of adc_to_sample to sample from. First channel is 1, 2nd is 2, etc.
uint32_t adc_channnel_to_sample = 1;

#define CHANNELS_PER_ADC 4
#define NUMBER_OF_ADCS 4


/*
 * Function that will send a status to the master unit
 * that basically says that everything on this particular
 * module is ok( temperature and voltage measurements are within ranges)
 *
 * TODO Determine what CAN message ID will be sent to back
 * that will act as the status flag for the time being.
 *      - This is not urgent, jsut something that needs to
 *      be decided on at some point
 */
void send_ok_state() {
    uint8_t priority = 1;
    uint8_t length = 8;
    uint8_t buf[8] = {1,2,3,4,5,6,7,8};
    uint8_t is_ext = 1;
    // CAN Message ID
    uint8_t msg = NODE_INFO_MSG_BASE_ID + NODE_INFO_MSG_ID_OFFSET*SLAVE_MODULE_ID ;
    can_send(msg, is_ext, buf, length, priority);

}

void send_voltage_error_state() {
    uint8_t priority = 1;
    uint8_t length = 8;
    uint8_t buf[8] = {1,1,1,1,1,1,1,1};
    uint8_t is_ext = 1;
    // CAN Message ID
    uint8_t msg = NODE_INFO_MSG_BASE_ID + NODE_INFO_MSG_ID_OFFSET*SLAVE_MODULE_ID ;
    can_send(msg, is_ext, buf, length, priority);

}

//TODO: Talk to tyler about what we want error states to look like
void send_temperature_error_state() {
    uint8_t priority = 1;
    uint8_t length = 8;
    uint8_t buf[8] = {2,2,2,2,2,2,2,2};
    uint8_t is_ext = 1;
    // CAN Message ID
    uint8_t msg = NODE_INFO_MSG_BASE_ID + NODE_INFO_MSG_ID_OFFSET*SLAVE_MODULE_ID ;
    can_send(msg, is_ext, buf, length, priority);

}

void send_temp_data() {
    uint32_t big_boi;
    uint32_t big_array[12];


    for(uint32_t i = 0; i < 12; i++){
        big_boi = 0;
        for(uint32_t j = 0; j < 5; j++){
            if (module_temperatures[i][j] > big_boi){
                big_boi = module_temperatures[i][j];
            }
        }
        big_array[i] = big_boi;
    }




    uint8_t priority = 2; //TODO: What priority should we use?
    uint8_t length = 12;
    uint8_t is_ext = 1;
    // CAN Message ID
    uint8_t msg = NODE_INFO_MSG_BASE_ID + NODE_INFO_MSG_ID_OFFSET*SLAVE_MODULE_ID ;
    can_send(msg, is_ext, big_array, length, priority);

}

void send_voltage_data() {
    uint8_t priority = 2;
    uint8_t length = 12;
    uint8_t is_ext = 1;
    // CAN Message ID
    uint8_t msg = NODE_INFO_MSG_BASE_ID + NODE_INFO_MSG_ID_OFFSET*SLAVE_MODULE_ID ;
    can_send(msg, is_ext, module_voltages, length, priority);

}

//Returns true if the voltage is in an acceptable range
bool voltage_is_acceptable(uint32_t voltage){
    if (VOLTAGE_HIGH_LIMIT > voltage > VOLTAGE_LOW_LIMIT){ //TODO: Change this so that it calculates the actual voltage and compares it to actual voltage mins and maxes
        return true;
    }
    return false;
}

//Function that handles voltages from the ADCs. Called by the ISR for reieving data
void handle_adc_voltage(uint32_t voltage, uint32_t adc_number, uint32_t channel_number){

    //Set flag if voltage is not acceptable
    if (!voltage_is_acceptable){
        module_error_state = 1;
    }

    //Record voltage data to array
    uint32_t offset = 3 * adc_number;
    module_voltages[offset + channel_number] = voltage;


    //TODO: Request next value from ADC
}



int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	

	/*
	 * INIT the can module for use
	 *
	 * Includes all of the init for the eUSCI_A SPI
	 * module and the MCP2515
	 */
    can_init();





	/*
	 *  Init the ADCs for use
	 */

    // Create the ADC object
    //adc_lib mcp3425();

    // Set the clocks that will be used for the i2c bus
    //mcp3425.set_clocks();

    // Initialize the i2c bus for use
    //adc.init_i2c_bus();




    /*
     * Init the timers that will sample the adcs at a given rate
     *
     * Timers needed.
     *
     * Interval | Purpose
     * 1 second | Send the module status
     * 40ms     | Send a new request to a ADC for their ADC conversion
     */


    //Start timer in continuous mode sourced by SMCLK
    Timer_A_initContinuousModeParam initContParam = {0};
    initContParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam.timerClear = TIMER_A_DO_CLEAR;
    initContParam.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam);

    //Initiaze CCR0 register for the 1 second timer
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_0
        );

    Timer_A_initCompareModeParam initCompParam = {0};
    initCompParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
    initCompParam.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    initCompParam.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    /*
     * Sets up the 1 second timer using the value of the SMCLK
     * that is the source of the timer module
     */
    initCompParam.compareValue = CS_getSMCLK();
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);



    Timer_A_startCounter( TIMER_A1_BASE,
            TIMER_A_CONTINUOUS_MODE
                );



	return 0;
}






#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCIB0_ISR (void)
#else
#error Compiler not supported!
#endif
{
   switch(__even_in_range(UCB0IV, 0x1E))  // double check these are the right bits****!!!!!
    {
      case USCI_NONE:          break;                     // Vector 0: No interrupts break;
      case USCI_I2C_UCALIFG:   break;                     // Vector 2: ALIFG break;
      case USCI_I2C_UCNACKIFG:
          //printf("NACK bit recieved!!!");
          break;                     // Vector 4: NACKIFG break;
      case USCI_I2C_UCSTTIFG:  break;                     // Vector 6: STTIFG break;
      case USCI_I2C_UCSTPIFG:  break;                     // Vector 8: STPIFG break;
      case USCI_I2C_UCTXIFG3:
//          if(config_index < 4)
//              EUSCI_B_I2C_masterSendMultiByteNext( EUSCI_B0_BASE,
//                                                   slave_config[config_index++] );
//          else
//              EUSCI_B_I2C_masterSendMultiByteNext( EUSCI_B0_BASE,
//                                                   slave_config[config_index] );
//              count_index = 0;
//          break;
      case USCI_I2C_UCTXIFG2:  break;   // each
      case USCI_I2C_UCTXIFG1:  break;
      case USCI_I2C_UCTXIFG0:  break;

      uint32_t voltage;
      uint32_t channel_number;

      case USCI_I2C_UCRXIFG3:    // slave 28 num3, voltage measurement
          voltage = 10000; //Placeholder value
          channel_number = 2; //Placeholder value
          handle_adc_voltage(voltage, 2, channel_number);
          break;

      case USCI_I2C_UCRXIFG2:// slave 28 num2, voltage measurement
          voltage = 10000; //Placeholder value
          channel_number = 2; //Placeholder value
          handle_adc_voltage(voltage, 1, channel_number);
          break;

      case USCI_I2C_UCRXIFG1:// slave 28 num1, voltage measurement
          voltage = 10000; //Placeholder value
          channel_number = 2; //Placeholder value
          handle_adc_voltage(voltage, 0, channel_number);
          break;

      case USCI_I2C_UCRXIFG0:  // slave 25, least priority, temperature

          /*
           * This handles measurement of temperatures
           */
          uint32_t temperature = 50; //Placeholder value

          uint32_t module_number = 0; //Placeholder value. From 0 to 11, represents which module this measurement is for
          uint32_t measurement_number = 0; //Placeholder value. From 0 to 4, represents which thermister in the module this measurement is for


          //Set the error flag if temperature is outside of range
          //TODO: Different setpoints for charging and discharging
          if (temperature > TEMPERATURE_LIMIT_DISCHARGING){
              module_error_state = 2;
          }

          //Record the temperature to the 2d array
          module_temperatures[module_number][measurement_number] = temperature;

          break;
          //break;

      case USCI_I2C_UCBCNTIFG: break;                     // Vector 28: BCNTIFG break;
      case USCI_I2C_UCCLTOIFG: break;                     // Vector 30: clock low timeout break;
      case USCI_I2C_UCBIT9IFG: break;                     // Vector 32: 9th bit break;
      default: break;
    }
}


//******************************************************************************
//
//This is the TIMER1_A0 interrupt vector service routine.
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A0_VECTOR)))
#endif
void TIMER1_A0_ISR (void)
{
    /*
     * Get the value of the CCR interrupts
     * Need to check which interrrupt was triggered
     */

    uint32_t interrupt_status = Timer_A_getInterruptStatus(TIMER_A1_BASE);


    //1 second timer
    if ( interrupt_status |= TIMER_A_CAPTURECOMPARE_REGISTER_0) {

        uint16_t compVal = Timer_A_getCaptureCompareCount(TIMER_A1_BASE,
                TIMER_A_CAPTURECOMPARE_REGISTER_0);

        //Send OK state if there's no errors
        if (module_error_state == 0){
            send_ok_state();
        }else if (module_error_state == 1){
            //Voltage error
            send_voltage_error_state();
        }else if (module_error_state == 2){
            //Temperature error
            send_temperature_error_state();
        }else{
            //The error state should never be anything except 0, 1, or 2
        }


        //Send data from module_voltages and module_temps
        send_temp_data();
        send_voltage_data();


        //Reset error flag
        module_error_state = 0;


        //Add Offset to CCR0
        Timer_A_setCompareValue(TIMER_A1_BASE,
            TIMER_A_CAPTURECOMPARE_REGISTER_0,
            compVal + CS_getSMCLK()
            );
    }
}


