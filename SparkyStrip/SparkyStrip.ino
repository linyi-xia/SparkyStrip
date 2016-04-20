
#include <AD_CHIP.h>
#include <SPI.h>
#include <Goertzel.h>

////////////////////////// User defined values and switches /////////////////////////////////
const float SECONDS = 1;
const float POWER_SCALAR = .27; 

const int AVE_COUNT = 4;

const float NOTHING_THRESHOLD = 0.1;
/*
#define WLAN_SSID       "UCInet Mobile Access"        // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_UNSEC
*/

//#define MAG_PHASE
//#define SWEEP
//#define DUMPER

// commented out to disable WIFI
#define WIFI

#ifdef SWEEP 
#undef WIFI 
#endif //SWEEP
#ifdef WIFI 
#include "arduino_mysql.h"
#include <ccspi.h>
  
  const char* WLAN_SSID = "JeffnTahe";        
  const char* WLAN_PASS  = "1loveTahereh!";
  // Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
  const int WLAN_SECURITY = WLAN_SEC_WPA2;
  
  //const IPAddress SERVER_ADDRESS(72,219,144,187);  // IP of the MySQL server
   const IPAddress SERVER_ADDRESS(169,234,209,144);

#endif

////////////////////////// should not change /////////////////////////////////
#define INTEGRATOR
#ifdef INTEGRATOR
  const byte CH1OS_VAL = 0x80;        
  const byte GAIN_VAL = 0x14;          //this one is the gain (page 14 of the datasheet)
#else
  const byte CH1OS_VAL = 0x00;
  const byte GAIN_VAL = 0x00;
#endif

const int VMODE = 0x608C;
const int IMODE = 0x408C;

const float POWER_RATIO =  0.827;   //per manual


const int HALF_CYCLES = 120*SECONDS;
const int WAVE_COUNT_MASK = 0x7;   // sample every 8 availible waveforms
const int MAX_SAMPLES = 3600*SECONDS;

#ifdef DUMPER
char zx[MAX_SAMPLES];
#endif
long samples[MAX_SAMPLES];

////////////////////////// Global resourses /////////////////////////////////

AD_Chip AD;  //instance of the class that handles the AD chip


////////////////////////////// Main Functions /////////////////////////////

void configAD(){
  AD.setup();
  AD.write_int(MODE, IMODE);  // sample current on ADC1 at 27.9kps
  AD.write_byte(CH1OS, CH1OS_VAL);    // integrator setting
  AD.write_byte(GAIN, GAIN_VAL);      // gain adjust
  
  AD.enable_irq(WSMP);    // New data is present in the waveform register.
  AD.enable_irq(ZX);      // zero crossing interrupt - we have the pin also, but sometimes it makes sense to read it here

  // wait few seconds for for readings to stabilize
  int count = 0;
  while( !AD.read_irq() || !AD.irq_set(ZX) || ++count < 600 );
}

//the setup function runs once when reset button pressed, board powered on, or serial connection
void setup() {
  Serial.begin(115200); 
  if(Serial)
    Serial.println(F("Booting!")); 
#ifdef WIFI
  pass_wifi_values(WLAN_SSID, WLAN_PASS, WLAN_SECURITY, SERVER_ADDRESS);
  while(!mysql_connect());  //loop until we connect to the server
#endif    
  configAD();
  
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
  float power_base, max_diff;
  float package[14] = {0};
  for(int outer_i = 0; outer_i < AVE_COUNT ; ++outer_i){
    int next_outer_i = outer_i+1;
    // reset our state
    int sample_count = 0;
    int waveform_count = 0;
    int half_cycle_count = 0;
  
    // if not a neg->pos zero crossing wait until we are on the next positive part
    while( digitalRead(AD_zx) );
    while( !digitalRead(AD_zx) );
    AD.read_irq(); // clear the interrupt status before starting our data grabbing loop
    AD.read_long(RAENERGY);  // clear the power accumulators
    AD.read_long(RVAENERGY);
    
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
        data |= digitalRead(AD_zx)<<23;  // put the ZX signal at the msb
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
    if( AD.read_int(MODE) != IMODE){
      if( Serial )
          Serial.println(F("AD chip reset detected, reconfiguring..."));
      configAD();
      return;
    }
    // we've reached our number of line-cycles so lets process the data
    process_data::N = sample_count;
    process_data::Sample_Rate = sample_count*1.0/SECONDS;
    float active = AD.read_long(RAENERGY) * (POWER_RATIO * POWER_SCALAR / SECONDS) ;
    float apparent = AD.read_long(RVAENERGY) * ( POWER_SCALAR / SECONDS);
    float reactive = sqrt(apparent*apparent - active*active);
#ifdef SWEEP
    Serial.println(sample_count);
    Serial.print(active);
    Serial.print(", "); 
    Serial.print(apparent);
    Serial.print(", "); 
    Serial.println(reactive);
    char buff[100];
    for(int i = 1; i < 12; i+=2){
      long target = i*60000;
      long top = target+60000;
      for( long f = target-60000; f < top;){
        process_data p(f/1000.0f);
        Goerzel_result gr = p.process_all(samples);
        sprintf(buff,"%f",f/1000.0f);
        Serial.print(buff);
        Serial.print(", "); 
        Serial.print(gr.amplitude() );
        Serial.print(", "); 
        Serial.print( gr.phase() );
        Serial.print(", "); 
        Serial.print( gr.real );
        Serial.print(", "); 
        Serial.print( gr.imaginary );
        Serial.println();
        double diff = abs(f-target);
        if( diff <= 10000){
          if( diff <= 1000){  
            if( diff <= 100 )
              f+=5;
            else
              f+=100;
          }
          else
            f+=500;
        }
        else
          f+=1000;
      }
    }
#else //SWEEP
///////// Normal operation here ///////

    if( isnan(reactive) )
       reactive = 0;
    if( outer_i == 0 ){
      power_base = active;
      if( power_base < 6)
        max_diff = 2.5;
      if( power_base < 15)
        max_diff = 1.5;
      else
        max_diff = 1;
    }
    else if(abs(power_base-active)> max_diff){
      if( Serial ){
        Serial.print(F("Power change of "));
        Serial.print(abs(power_base-active));
        Serial.println(F(" detected, Starting over..."));
      }
      return;
    }
    /*Serial.print(active);
    Serial.print(", "); 
    Serial.println(active-power_base);
    */
    
#ifdef MAG_PHASE
    Serial.println();
    Serial.print(next_outer_i);
    Serial.print(", "); 
    Serial.println(sample_count);
    Serial.print(active);
    Serial.print(", "); 
    Serial.print(apparent);
    Serial.print(", "); 
    Serial.println(reactive);
#endif //MAG_PHASE
    package[0] = (package[0]*outer_i + active) / next_outer_i;
    package[1] = (package[1]*outer_i + reactive) / next_outer_i;
    int p_i = 2;
    for(int i = 1; i < 12; i+=2){
      process_data p(i*60);
      Goerzel_result gr = p.process_all(samples);
      package[p_i] = (package[p_i]*outer_i + gr.real) / next_outer_i;
      ++p_i;
      package[p_i] = (package[p_i]*outer_i + gr.imaginary) / next_outer_i;
      ++p_i;
#ifdef MAG_PHASE
      Serial.print(i*60);
      Serial.print(", "); 
      Serial.print(gr.amplitude() );
      Serial.print(", "); 
      Serial.print( gr.phase() );
      Serial.print(", "); 
      Serial.print( gr.real );
      Serial.print(", "); 
      Serial.print( gr.imaginary );
      Serial.println();
#endif //MAG_PHASE
    }
    if(next_outer_i == AVE_COUNT){
#ifdef WIFI
      send_mysql(package);
#else  //WIFI
      if( Serial ){
        for(int i = 0; i<14; ++i){
          Serial.print(package[i]);
          if(i != 13)
            Serial.print(", "); 
          else
            Serial.println();
        }
      }
#endif //WIFI
    }
#endif //SWEEP
}
  
}

