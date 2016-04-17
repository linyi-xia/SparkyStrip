
#ifndef AD_CHIP_H
#define AD_CHIP_H

/*
Older setup:
AD     Arduino    signal
2         11     DIN <- MOSI
3         9      reset <- low (not reset)
4         4      CS
5         13     Clock
10        2      IRQ -> interupt handler (waveform sampling)
13        12     DOUT -> MISO

Newer:
Arduino             AD Board
2                       U7 pin 1    (IRQ)
9                       U5 pin 7    (Reset)
4                     U3 pin 11  (CS)
11 or ICSP-4    U3 pin 3     (MOSI)
12 or ICSP-1    DOUT test point (with alligator clip)   (MISO)
13 or ICSP-3    U3 pin 8     (Clock)

where ICSP is listed both pins are connected on the uno so either one works, 
must be the ICSP/SPI header on the DUE. Pin 1 of that header is the corner closest to the "ON" led.

*/


#include "Arduino.h"
#include "constants.h"

//block below must be copied to the .ino file
#include <SPI.h>
#if defined(__arm__) && defined(__SAM3X8E__) // Arduino Due
  #define DUE 
  //#include "DueFlashStorage.h"
#else
  //#include <EEPROM.h>
#endif

//uncomment the line below to use my bit-banging SPI implimentation
//#define own_spi


long extend(long signed24){
  if( signed24 & 1<<23 )
    return signed24|(255<<24);
}

////////////////////////////// The Class definition ///////////////////////////////

class AD_Chip{
public:
  void setup();
  void reset();
  // TODO: determine needed size of message_goes_here;
//void read_reg(byte reg, char* message); //modifies message for sending back via serial.

  // NOTE: these do no checking!
  long read_waveform();
  long read_long(byte reg);  //used for getting the waveform or other 24 bit registers (sign extended if register signed)
  int read_int(byte reg);    //used for getting 12-16 byte registers
  byte read_byte(byte reg);   //used for getting 6-8 byte registers

//void write_reg(byte reg, char* message);
  void write_int(byte reg, int data);    //used for writing 12-16 byte registers
  void write_byte(byte reg, byte data);   //used for writing 6-8 byte registers
  
  // TODO: add functions for setting all possible settings
  
  bool read_irq();            // reads the interrupt pin and if set reads the interrupt register and returns true
  bool irq_set(byte irq);     // returns true if the interrupt asked for is set
  void disable_all_irq();     // clears them all
  void enable_irq(int irq);  // enable one interrupt
  void disable_irq(int irq); // disable one interrupt
  
  
    
  // wraps SPI.transfer so we can use the same syntax in all cases and build in delay management (wait if needed)
  byte transfer(byte message, bool cont = false, bool delay = true); //if delay = true, transfer will ensure 4us between transfers
  
  // old methods to be phased out
  // *warning* format is strange - message[0] is register, message[1] is MSB (and integers are little endian)
  void write(byte *message, byte reg_size);
//  void read(byte *message, byte reg_size);
//private:
  int irq_status;
};



#if defined(__arm__) && defined(__SAM3X8E__) // Arduino Due
  #define DUE 
  //DueFlashStorage dueFlashStorage;
#endif

void SI(int data){
  byte* acc = (byte*)&data;
  //Serial.write(acc[1]);  
  //Serial.write(acc[0]);  
}


inline 
void save(byte loc, byte data){
#ifdef DUE
  //dueFlashStorage.write(loc,data);
#else
  //EEPROM.write(loc,data);
#endif
}

inline 
byte load(byte loc){
#ifdef DUE
  //return dueFlashStorage.read(loc);
#else
  //return EEPROM.read(loc);
#endif
}

// set all the eeprom locations for registry values to defaults
void initalize_eeprom(){
    // default values for all the writable registers
    //Serial.write("Setting default values to EEPROM\n");
    save(1,0x58); //MODE
    save(2,0x0c);
    save(3,0x00); //IRQEN
    save(4,0x40);
    save(5,0x00); //CH10S
    save(6,0x00); //CH20S
    save(7,0x00); //GAIN
    save(8,0x0D); //PHCAL  (6 bits right alligned)
    save(9,0x00); //APOS
    save(10,0x00);
    save(11,0x00); //WGAIN (12 bits right alligned)
    save(12,0x00);
    save(13,0x00); //WDIV
    save(14,0x00); //CFNUM (12 bits right alligned)
    save(15,0x3F);
    save(16,0x00); //CFDEN (12 bits right alligned)
    save(17,0x3F);
    save(18,0x00); //IMRMSOS (12 bits right alligned)
    save(19,0x00);
    save(20,0x00); //VRMSOS (12 bits right alligned)
    save(21,0x00);
    save(22,0x00); //VAGAIN (12 bits right alligned)
    save(23,0x00);
    save(24,0x00); //VADIV
    save(25,0xFF); //LINECYC
    save(26,0xFF);
    save(27,0x0F); //ZXTOUT (12 bits right alligned)
    save(28,0xFF);
    save(29,0xFF); //SAGECYC
    save(30,0x00); //SAGLVL
    save(31,0xFF); //IPKLVL
    save(32,0xFF); //VPKLVL
    save(0,0xAA);  //Flag incicating these are set
}


void irq_state()
{
    Serial.print(micros());
    if(digitalRead(AD_interrupt_pin))
        Serial.print(" HIGH\n");
    else
        Serial.print(" LOW\n");
}

void AD_Chip::setup(){
    // initalize pins:
  pinMode(AD_reset, OUTPUT);
  pinMode(AD_interrupt_pin, INPUT);
  digitalWrite(AD_reset, LOW); //reset the AD chip
#ifndef DUE
  pinMode(AD_cs_pin, OUTPUT);
  digitalWrite(AD_cs_pin, HIGH);
#endif
  

#ifdef own_spi
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
#elif defined(DUE)
  SPI.begin(AD_cs_pin);
  SPI.setClockDivider(AD_cs_pin, 50);
  SPI.setBitOrder(AD_cs_pin, MSBFIRST);
  SPI.setDataMode(AD_cs_pin, SPI_MODE1);
  
#else
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);  //DIV2 does not work
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
  
#endif
  //setup an interrupt for the AD chips interrupt output (active low)
  //attachInterrupt(0,AD_interrupt,LOW);
  digitalWrite(AD_reset, HIGH); //turn the AD chip on (not reset)
//  while(digitalRead(AD_interrupt_pin))
//      Serial.write("delay");  
  //check if our EEPROM is set yet
  //if(load(0) != 0xAA)
    //initalize_eeprom();
    
  //wait for the AD chip to warm up then initialize the board to our settings
  delay(3000);
  //byte eeprom_loc;
  //byte data[max_reg_size+1];          //create a byte array to hold the message
  // work through each register and send our set values to the AD board if they are writable registers
/*
#ifndef DUE  
  for(int i=0; i<= last_REG; ++i){
    eeprom_loc = REG_EEPROM_loc[i];
    if(eeprom_loc) {
      data[0] = i;
      byte reg_size = REG_bytes[i];     //find the register size
      for(int j = 0; j<reg_size; ++j)   //for the number of bytes of data
        data[j+1] = load(eeprom_loc+j);  //fill the array
      Serial.write(data,reg_size+1);
      write(data, reg_size);    //write it to the AD_cs_pin chip
    }
  }
#endif
*/
}

void AD_Chip::reset(){
  digitalWrite(AD_reset, LOW); //put the chip into reset
  delay(500);
  digitalWrite(AD_reset, HIGH); //turn the AD chip on (not reset)
  delay(500);
}

//no delay needed for reading the waveform
inline
long AD_Chip::read_waveform(){
  //TODO: make this generic, for now assumes a 24 bit signed register
  union{
    long as_long;
    byte as_bytes[4];
  }data;
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, WAVEFORM, SPI_CONTINUE);
  delayMicroseconds(1);
  data.as_bytes[2] = SPI.transfer(AD_cs_pin, 0x00, SPI_CONTINUE);        //swap endian order
  data.as_bytes[1] = SPI.transfer(AD_cs_pin, 0x00, SPI_CONTINUE);
  data.as_bytes[0] = SPI.transfer(AD_cs_pin, 0x00);
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(WAVEFORM);
  data.as_bytes[2] = SPI.transfer(0x00);        //swap endian order
  data.as_bytes[1] = SPI.transfer(0x00);
  data.as_bytes[0] = SPI.transfer(0x00);
  digitalWrite(AD_cs_pin, HIGH);
#endif
  interrupts();
  if(data.as_bytes[2]&0x80) //if msb set
    data.as_bytes[3]=0xFF;  //and sign extend
  else
    data.as_bytes[3]=0x00;
  return data.as_long;
}

inline
long AD_Chip::read_long(byte reg){
  //TODO: make this generic, for now assumes a 24 bit signed register
  union{
    long as_long;
    byte as_bytes[4];
  }data;
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, reg, SPI_CONTINUE);
  delayMicroseconds(4);
  data.as_bytes[2] = SPI.transfer(AD_cs_pin, 0x00, SPI_CONTINUE);        //swap endian order
  data.as_bytes[1] = SPI.transfer(AD_cs_pin, 0x00, SPI_CONTINUE);
  data.as_bytes[0] = SPI.transfer(AD_cs_pin, 0x00);
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(reg);
  delayMicroseconds(4);
  data.as_bytes[2] = SPI.transfer(0x00);        //swap endian order
  data.as_bytes[1] = SPI.transfer(0x00);
  data.as_bytes[0] = SPI.transfer(0x00);
  digitalWrite(AD_cs_pin, HIGH);
#endif
  interrupts();
  if(data.as_bytes[2]&0x80) //if msb set
    data.as_bytes[3]=0xFF;  //and sign extend
  else
    data.as_bytes[3]=0x00;
  return data.as_long;
}

int AD_Chip::read_int(byte reg){
  //TODO: make this generic, for now assumes a 12-18 bit register
  union{
    int as_int;
    byte as_bytes[2];
  }data;
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, reg, SPI_CONTINUE);
  delayMicroseconds(4);
  data.as_bytes[1] = SPI.transfer(AD_cs_pin, 0x00, SPI_CONTINUE);
  data.as_bytes[0] = SPI.transfer(AD_cs_pin, 0x00);
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(reg);
  delayMicroseconds(4);
  data.as_bytes[1] = SPI.transfer(0x00);
  data.as_bytes[0] = SPI.transfer(0x00);
  digitalWrite(AD_cs_pin, HIGH);
#endif
  interrupts();
  return data.as_int;
}

byte AD_Chip::read_byte(byte reg){
  //TODO: make this genaric, for now assumes a 6-8 bit register
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, reg, SPI_CONTINUE);
  delayMicroseconds(4);
  reg = SPI.transfer(AD_cs_pin, 0x00);
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(reg);
  delayMicroseconds(4);
  reg = SPI.transfer(0x00);
  digitalWrite(AD_cs_pin, HIGH);
#endif
  interrupts();
  return reg;
}

inline
void AD_Chip::write_int(byte reg, int data){
  byte *data_bytes = (byte*)&data;
  reg |= 0x80;  //setting the write bit
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, reg, SPI_CONTINUE);
  delayMicroseconds(4);
  SPI.transfer(AD_cs_pin, data_bytes[1], SPI_CONTINUE);
  delayMicroseconds(4);
  SPI.transfer(AD_cs_pin, data_bytes[0]);
  interrupts();
  delayMicroseconds(4);  //on write we need a minimum delay of 4us, Due needs this to ensure it
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(reg);
  delayMicroseconds(3);
  SPI.transfer(data_bytes[1]);
  delayMicroseconds(3);
  SPI.transfer(data_bytes[0]);
  digitalWrite(AD_cs_pin, HIGH);
  interrupts();
#endif

  //save what was set
  reg = REG_EEPROM_loc[reg];
  save(reg,data_bytes[1]);  
  save(reg+1,data_bytes[0]);  //int is little endian
}

inline
void AD_Chip::write_byte(byte reg, byte data){
  reg |= 0x80;  //setting the write bit
  noInterrupts();
#ifdef DUE
  SPI.transfer(AD_cs_pin, reg, SPI_CONTINUE);
  delayMicroseconds(4);
  SPI.transfer(AD_cs_pin, data);
  interrupts();
  delayMicroseconds(4);  //on write we need a minimum delay of 4us, Due needs this to ensure it
#else
  digitalWrite(AD_cs_pin, LOW);
  SPI.transfer(reg);
  delayMicroseconds(3);
  SPI.transfer(data);
  digitalWrite(AD_cs_pin, HIGH);
  interrupts();
#endif
  
  reg = REG_EEPROM_loc[reg];
  save(reg,data);  
  
}

inline
bool AD_Chip::read_irq(){
  if(digitalRead(AD_interrupt_pin))
    return false;
  irq_status = read_int(RSTSTATUS);
  return true;
}

inline
bool AD_Chip::irq_set(byte irq){
  //Serial.write(0xAA);
  return irq_status & 1<<irq;
}
  
inline
void AD_Chip::disable_all_irq(){
  write_int(IRQEN,0);
}

inline
void AD_Chip::enable_irq(int irq){
  int irq_enable = read_int(IRQEN);
  irq = 1<<irq;
  irq_enable |= irq; //set the bit
  write_int(IRQEN, irq_enable);
}

inline
void AD_Chip::disable_irq(int irq){
  int irq_enable = read_int(IRQEN);
  irq = 1<<irq;
  irq ^= -1; //invert the mask
  irq_enable &= irq; //clear the bit
  write_int(IRQEN, irq_enable);
}

inline
void AD_Chip::write(byte *message, byte reg_size){
  noInterrupts();
   //find the eeprom location for this register to keep it up to date
  byte eeprom_loc = REG_EEPROM_loc[message[0]];

  // build the message to the communications register by marking bit 7 in reg to 1
  message[0] |= 0x80;  //setting the send bit
  
  transfer(message[0], true);
  save(eeprom_loc,message[1]);  //write to eeprom (here for built in delay we need)
  transfer(message[1], true);
  if(reg_size = 2){
    transfer(message[2], false, false);
    save(eeprom_loc+1,message[2]);
  }
  interrupts();
}
/*
//message includes the register then space for data, but reg_size is just the size of the data
void AD_Chip::read(byte* message, byte reg_size) {
    // for read the message to communication register is just the register we want to talk to
    // so we don't modify reg
    // take the chip select low to select the device:
    transfer(*message++, true); //Send register location
    for(byte *top=message+reg_size; message<top; ++message){
        *message = SPI.transfer(0x00, true);  //just read data back
    }
}
*/
inline
byte AD_Chip::transfer(byte data, bool cont, bool delay){
  //Serial.write(data);
  static long last_time; 
  byte rcv = 0;
  //Serial.println(micros()-last_time);
  if(delay){
    last_time -= micros() - 4;
    if(last_time > 0)
      delayMicroseconds(last_time);
  }
#ifndef DUE
  digitalWrite(AD_cs_pin, LOW);
#endif
  //take the chip select low to select the device:
  
#ifdef own_spi
  byte clk = 0;
  byte mask = 0x80; //start with MSB
  for(byte i = 16; i>0; --i){
      clk ^= 1;
      if(clk){ //even numbers - when clock pulse is to be high
          if(i != 16) //if not the first cycle, shift our mask
              mask>>=1;
          digitalWrite(SCK, HIGH);    //set the clock pulse
          digitalWrite(MOSI, data&mask);   //write our data
          //for our verifications
      }
      else{
        if(digitalRead(MISO))  //read the data
            rcv |= mask;
          digitalWrite(SCK, LOW); //unset the clock pulse
      }
  }
  
#elif defined(DUE)  
  if(cont)
    rcv = SPI.transfer(AD_cs_pin, data, SPI_CONTINUE);
  else
    rcv = SPI.transfer(AD_cs_pin, data);
#else // own_spi
  rcv = SPI.transfer(data);
#endif // own_spi

#ifndef DUE
  if(!cont)
    digitalWrite(AD_cs_pin, HIGH);
#endif // Due
  //Serial.println("");
  last_time = micros();
  return rcv;
}  

  
#endif //AD_CHIP_H
