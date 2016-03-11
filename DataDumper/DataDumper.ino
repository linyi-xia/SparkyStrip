
#include "settings.h"
#include "AD_CHIP.h"
#include <SPI.h>

////////////////////////// Global resourses /////////////////////////////////
typedef void (*func_ptr)();

AD_Chip AD;  //instance of the class that handles the AD chip
func_ptr run;

/////////////////////// Serial input processing ////////////////////////////

//enum only used by serial event
enum {get_reg, get_1_2, get_2_2, get_1}serial_state = get_reg;

void serialEvent() {
    static byte reg, data;
    while(Serial.available()){
        byte new_data = Serial.read();
        //Serial.write(new_data);
        switch(serial_state){
            case get_reg:
            {
                if(new_data == 0x00){
                    if( run == nothing )
                      Serial.println("Standby");
                    else
                      Serial.println("Running");
                }
                if(new_data == 0x01){
                    long l_data = AD.read_waveform();
                    byte* access = (byte*)&l_data;
                    Serial.write(access[2]);  //reverse endian order
                    Serial.write(access[1]);
                    Serial.write(access[0]);
                    return;
                }
                if(new_data == 0xAA){
                    AD.write_byte(CH1OS,0x00);
                    AD.write_byte(CH1OS,ch1os);
                    if( run != nothing ){
                      Serial.println("Going to idle mode");
                      run = nothing;
                    }
                    else{
                      run = sample_current;
                    }
                    return;
                }
                if(new_data == 0xAB){
                    run = sample_voltage;
                    return;
                }
                if(new_data == 0xAC){
                    run = sample_current;
                    return;
                }
                reg = new_data & 0x7F; //save it without the write bit set
                if(reg > last_REG){
                    Serial.write(0xA1);  //send an error code back if this value is out of range
                    //Serial.write(reg);
                    break;            //and do nothing with this byte
                }
                byte reg_size = REG_bytes[reg];
                if(new_data & 0x80){     //was write indicated?
                    switch(reg_size){    //if so set the next state by the data_size
                        case 1:
                            serial_state = get_1;
                            break;
                        case 2:
                            serial_state = get_1_2;
                            break;
                        default:       //if the value is not 1 or 2 then this is not a register we write to
                            Serial.write(0xA2);  //so send an error code back (and do nothing with this byte)
                    }
                }
                else {  //this is a read so no other data to recieve, so lets read the data and send it back
                    Serial.write(reg);
                    if(reg_size == 3){
                        long long_data = AD.read_long(reg);
                        byte* access = (byte*)&long_data;
                        Serial.write(access[2]);  //reverse endian order
                        Serial.write(access[1]);
                        Serial.write(access[0]);
                    }
                    else if(reg_size == 2){
                        int int_data = AD.read_int(reg);
                        byte* access = (byte*)&int_data;
                        Serial.write(access[1]);  //reverse endian order
                        Serial.write(access[0]);
                    }
                    else{
                        data = AD.read_byte(reg);
                        Serial.write(data);
                    }
                }
            }
                break;
            case get_1_2:
                data = new_data;
                serial_state = get_2_2;
                break;
            case get_2_2:
                union{
                    int i;
                    byte b[2];
                }d2;
                d2.b[1]=data;
                d2.b[0]=new_data;
                AD.write_int(reg,d2.i);
                serial_state = get_reg;
                reg &= 0x7f;
                delayMicroseconds(4);
                Serial.write(reg);
                d2.i = AD.read_int(reg);
                Serial.write(d2.b[1]);
                Serial.write(d2.b[0]);
                break;
            default:
                AD.write_byte(reg,new_data);
                serial_state = get_reg;
                reg &= 0x7f;
                delayMicroseconds(4);
                Serial.write(reg);
                Serial.write(AD.read_byte(reg));
                break;
        }
    }
    
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// working area  /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void nothing(){}


void sample_voltage(){
    Serial.println("Voltage:");
    AD.write_int(MODE, vMode);  // sample voltage on ADC1
    AD.read_long(RAENERGY);
    AD.read_long(RVAENERGY);
    run = dump;
}

void sample_current(){
    Serial.println("Current:");
    AD.write_int(MODE, iMode);  // sample voltage on ADC1
    AD.read_long(RAENERGY);
    AD.read_long(RVAENERGY);
    run = dump;
}

void dump(){
    
    if(!AD.read_irq())
        return;
    if(!AD.irq_set(WSMP))
        return;
    long sample = AD.read_waveform();
    long active = AD.read_long(AENERGY);
    long apparent = AD.read_long(VAENERGY);
    Serial.write(0xAA);
    Serial.write(sample);
    Serial.write(active);
    Serial.write(apparent);
}

////////////////////////////// Main Functions /////////////////////////////



//the setup function runs once when reset button pressed, board powered on, or serial connection
void setup() {
    Serial.begin(115200);  
    if(Serial)
      Serial.println("Booting!");
 
    AD.setup();
    
    AD.write_int(LINECYC, lineCyc); // make line-cycle accumulation produce power readings
    AD.write_byte(CH1OS, ch1os);    // integrator setting
    AD.write_byte(GAIN, gain);      // gain adjust
    AD.enable_irq(WSMP);
    AD.enable_irq(CYCEND);
    if(Serial)
        Serial.print("Ready to begin!\n");
    run = nothing;
}

// the loop function runs over and over again forever
void loop(){
    run();
}

