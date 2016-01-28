#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <math.h>


// pins used for AD chip comms
// other pins are controlled by the SPI library:
#define AD_interrupt_pin  2  //must be 2 or 3 for uno hardware interrupts (anywhere on due)
#define AD_reset          9  //logic low is reset
#define AD_cs_pin         4 //must be 4, 10 or 52 for due

////// registers///////
#define last_REG      0x27 //that we care about
#define max_reg_size  3    //bytes

//TODO: add all the registers
#define WAVEFORM  0x01 //waveform sample
#define AENERGY   0x02
#define RAENERGY  0x03 //read and clear active energy register
#define LAENERGY  0x04 //line accumulated RAENERGY
#define VAENERGY  0x05
#define RVAENERGY 0x06 //read and clear apparent energy register
#define LVAENERGY 0x07 // line accumulated RVAENERGY
#define MODE      0x09
#define STATUS    0x0B
#define IRQEN     0x0A //interrupt enable register
#define RSTSTATUS 0x0C  //register to read interrupts and clear them on read
#define CH1OS     0x0D
#define CH2OS     0x0E
#define GAIN      0x0F
#define PHCAL     0x10
#define APOS      0x11
#define WGAIN     0x12
#define WDIV      0x13
#define CFNUM     0x14
#define CFDEN     0x15
#define IRMS      0x16
#define VRMS      0x17
#define IRMSOS    0x18
#define IRMSOS    0x19
#define VAGAIN    0x1A
#define VRMSOS    0x19
#define VAGAIN    0x1A
#define VADIV     0x1B
#define LINECYC   0x1C //set number of half cycles for LAENERGY and LVAENERGY
#define RSTIPEAK  0x23 //read and clear peak current
#define RSTVPEAK  0x25 //read and clear peak voltage


//// interrupt bits (# of bits to left shift 0x0001 to make the mask) ////
#define AEHF    0 //The active energy register, AENERGY, is more than half full.
#define SAG     1 //Sag on the line voltage.
#define CYCEND  2 //End of energy accumulation over an integral number of half line cycles.
#define WSMP    3 //New data is present in the waveform register.
#define ZX      4 //Set to 1 on the rising and falling edge of the voltage waveform.
#define TEMP    5 //A temperature conversion result is available in the temperature register.
#define RESET   6 //Indicates the end of a reset for software and hardware resets -cannot be enabled to cause an interrupt.
#define AEOF    7 //The active energy register has overflowed.
#define PKV     8 //The waveform sample from Channel 2 has exceeded the VPKLVL value.
#define PKI     9 //The waveform sample from Channel 1 has exceeded the IPKLVL value
#define VAEHF   10 //The apparent energy register, VAENERGY, is more than half full.
#define VAEOF   11 //The apparent energy register has overflowed.
#define ZXTO    12 //Missing zero crossing on the line voltage for a specified number of line cycles
#define PPOS    13 //Power has gone from negative to positive.
#define PNEG    14 //power has gone from positive to negative.

////////////////////////////// Constants ////////////////////////////////////

// # of bytes for each register
const byte REG_bytes[] =
//         5         A         F           14           19             1E            23     27
{0,3,3,3,3,3,3,3,0,2,2,2,2,1,1,1,1,2, 2,1, 2 ,2 ,3,3, 2, 2, 2,1 ,2 , 2,1 , 1, 1, 1,3,3 ,3,3,1,2};

// type of each register


// EEPROM location storing values for writable registers - 0 indicates not writeable
// note that any odd sized register data size (eg: 6 or 12 bits) must be stored with the data to the MSB end
const byte REG_EEPROM_loc[] =
//         5         A         F           14           19             1E            23     27
{0,0,0,0,0,0,0,0,0,1,3,0,0,5,6,7,8,9,11,13,14,16,0,0,18,20,22,24,25,27,29,30,31,32,0,0 ,0,0,0,0};

///////////////////////// Default register values /////////////////////////////

#endif //CONSTANTS_H
