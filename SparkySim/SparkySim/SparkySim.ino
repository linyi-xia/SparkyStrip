

#include "SimSwitch.h"
#include "Goertzel.h"

#ifdef SIMULATION
#include "Serial.hpp"
#include "AD_Sim.hpp"
#else
#include "AD_CHIP.h"
#include <SPI.h>
#endif

AD_Chip AD;
Serial_sim Serial;
long samples[7200];
int count;

void setup(){
    PAST_DATA = samples+7199;
    Serial.begin(115200);
    if(Serial)
        Serial.println("Booting!");
#ifdef WIFI
    while(!mysql_connect());  //loop until we connect to the server
#endif
    AD.setup();
    
    AD.write_int(MODE, iMode);  // sample current on ADC1
    AD.write_int(LINECYC, lineCyc); // make line-cycle accumulation produce power readings
    AD.write_byte(CH1OS, ch1os);    // integrator setting
    AD.write_byte(GAIN, gain);      // gain adjust
    
    AD.enable_irq(CYCEND);  //End of energy accumulation over an integral number of half line cycles.
    AD.enable_irq(WSMP);    //New data is present in the waveform register.
    AD.enable_irq(ZX);      //rising and falling edge of the voltage waveform.
    
    if(Serial)
        Serial.print("Ready to begin!\n");
    
    count = 0;
    while(!AD.read_irq() || !AD.irq_set(CYCEND));
}

void loop(){
    if(!AD.read_irq() || !AD.irq_set(WSMP))
        return;
    samples[count++] = AD.read_waveform();
    if(!AD.irq_set(CYCEND))
        return;
    
    // at this point we have a complete dataset of lineCyc half cycles
    float package[9];
    package[0] = AD.read_long(LAENERGY) * power_ratio * power_scaler;
    int p_i = 1;
    Goerzel_result gr;
    process_data p[4] = { {60,count},{180,count},{300,count},{420,count} };
    for(int i = 0; i < 4; ++i){
        gr = p[i].process_all(samples);
        package[p_i++] = gr.real;
        package[p_i++] = gr.imaginary;
        /*Serial.print(gr.amplitude());
        Serial.print(',');
        Serial.print(gr.phase());
        Serial.print(',');
    }
    Serial.print('\n');
    */
}
    for(int i = 0; i<9; ++i){
        Serial.print(package[i]);
        Serial.print(',');
    }
    Serial.print('\n');

    count = 0;
    
}








