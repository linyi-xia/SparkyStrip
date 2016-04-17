
#include <settings.h>
#include <AD_CHIP.h>
#include <SPI.h>
#include <Goertzel.h>

// commented out to disable WIFI
#define WIFI

#ifdef WIFI
#include "arduino_mysql.h"
#include <ccspi.h>
#endif

////////////////////////// Global resourses /////////////////////////////////


#define SECONDS 1
#define HALF_CYCLES (int)(120*SECONDS)      
#define WAVE_COUNT_MASK 7   // sample every 8 availible waveforms
#define MAX_SAMPLES (int)(3600*SECONDS)

//#define DUMPER
//#define MAG_PHASE
//#define SWEEP

#ifdef DUMPER
char zx[MAX_SAMPLES];
#endif
long samples[MAX_SAMPLES];
AD_Chip AD;  //instance of the class that handles the AD chip


////////////////////////////// Main Functions /////////////////////////////


//the setup function runs once when reset button pressed, board powered on, or serial connection
void setup() {
  Serial.begin(115200); 
  if(Serial)
    Serial.println(F("Booting!")); 
#ifdef WIFI
  while(!mysql_connect());  //loop until we connect to the server
#endif    
  AD.setup();
  AD.write_int(MODE, 0x408C);  // sample current on ADC1 at 27.9kps
  AD.write_int(LINECYC, HALF_CYCLES);   // 2 seconds
  AD.write_byte(CH1OS, ch1os);    // integrator setting
  AD.write_byte(GAIN, gain);      // gain adjust
  
  AD.enable_irq(CYCEND);  //End of energy accumulation over an integral number of half line cycles.
  AD.enable_irq(WSMP);    //New data is present in the waveform register.
  AD.enable_irq(ZX); 
  pinMode(A5, INPUT);
  // wait a couple line cycles for things to stabilize
  while( !AD.read_irq() || !AD.irq_set(CYCEND) );
  while( !AD.read_irq() || !AD.irq_set(CYCEND) );
#ifdef  DUMPER
  Serial.print(F("Begin DataDumper!\n"));
  Serial.print( AD.read_long(LAENERGY) / SECONDS * power_ratio * power_scaler );
  Serial.print(", ");       
  Serial.println( AD.read_long(LVAENERGY) / SECONDS * power_scaler ); 
#else
  if(Serial)
    Serial.print(F("Begin SparkyStrip!\n"));
#endif
}



// runs over and over again forever
void loop(){
  // reset our state
  int sample_count = 0;
  int waveform_count = 0;
  int half_cycle_count = 0;

  // if not a neg->pos zero crossing wait until we are on the next positive part

  while( digitalRead(A5) );
  while( !digitalRead(A5) );
  
  // our "gather data" loop
  while(1){
    // poll for the next irq
    while( !AD.read_irq() );
    
    // if we have a new waveform availible grab it if our count is divisable by 8
    if( AD.irq_set(WSMP) && !(++waveform_count & WAVE_COUNT_MASK)){
      long data = AD.read_waveform();
      samples[sample_count++] = data; 
#ifdef DUMPER
      data &= ~(1<<23);             // clear the msb
      data |= digitalRead(A5)<<23;  // put the ZX signal at the msb
      Serial.write((char*)&data, 3); // spit it out - 3 bytes as that is all Serial can handle realtime
    }
#else    
    }
    // if we are at a zero crossing up our count
    // and if that new count equals our target number of half cycles break the "gather data" loop
    if( AD.irq_set(ZX) ){
      ++half_cycle_count;
      if(half_cycle_count == HALF_CYCLES )
        break;
    }
#endif  //DUMPER
  }

  // we've reached our number of line-cycles so lets process the data
  float package[9];
  package[0] = 1000*AD.read_long(LAENERGY) / SECONDS * power_ratio * power_scaler;
  int p_i = 1;
  process_data::N = sample_count;
  process_data::Sample_Rate = sample_count*1.0/SECONDS;
#ifdef SWEEP
  for(int i = 1; i < 12; i+=2){
    int top = i*60+60;
    for( float f = i*60-60; f < top;){
      process_data p(f);
      Goerzel_result gr = p.process_all(samples);
      Serial.print(f);
      Serial.print(", "); 
      Serial.print(gr.amplitude() );
      Serial.print(", "); 
      Serial.print( gr.phase() );
      Serial.println();
      if( abs(f-i*60) < 10){
        if( abs(f-i*60) < 1)  
          f+=.01;
        else
          f+=.1;
      }
      else
        f+=1;
    }
  }
#else //SWEEP
///// Normal operation here ////
  for(int i = 1; i < 8; i+=2){
    process_data p(i*60);
    Goerzel_result gr = p.process_all(samples);
#ifdef MAG_PHASE
    package[p_i++] = gr.amplitude();
    package[p_i++] = gr.phase();
#else
    package[p_i++] = gr.real;
    package[p_i++] = gr.imaginary;
#endif //MAG_PHASE
  }
  
#ifdef WIFI
  send_mysql(package);
#else
  if( Serial ){
    for(int i = 0; i<9; ++i){
      Serial.print(package[i]);
      if(i != 8)
        Serial.print(", "); 
      else
        Serial.print('\n');
    }
  }
#endif //WIFI
#endif //SWEEP

  
}

