
#include <AD_CHIP.h>
#include <SPI.h>
#include <Goertzel.h>
#include <IPAddress.h>
#include <common.h>
#include <watchdog.h>

////////////////////////// User defined values and switches /////////////////////////////////

// power scalar
const float POWER_SCALAR = .376; 
// zero adjust
const float ACTIVE_ZERO_ADJUST = 0;
const float APPARENT_ZERO_ADJUST = 0;

// how long we sample the waveform before calculating the values
const float SECONDS = 1;
// how many calculations to average before sending the data to the server
const int AVE_COUNT = 4;
// set the sample rate by this divider - native is 27.9k so divide by 8 for 3.5k
const int SAMPLE_DIV = 8; 

// switches for special modes
//#define MAG_PHASE
//#define SWEEP
//#define DUMPER

// commented out to disable WIFI
#define WIFI

//////// Access point config //////////

#define WLAN_SSID       "UCInet Mobile Access"        // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_UNSEC

/*
  #define WLAN_SSID "JeffnTahe"
  #define WLAN_PASS "1loveTahereh!"
  // Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
  #define WLAN_SECURITY WLAN_SEC_WPA2
*/

////////// MySQL server address ////////////
// 72.219.144.187
const IPAddress SERVER_ADDRESS(72,219,144,187);  // IP of the MySQL server
//const IPAddress SERVER_ADDRESS(169,234,209,144);

////////////////////////// should not change /////////////////////////////////

//both these modes don't use wifi so turn it off if it was enabled
#if defined(SWEEP) || defined(DUMPER) 
#undef WIFI 
#endif

#ifdef WIFI 
#include "arduino_mysql.h"
#include <ccspi.h>
#endif

#define INTEGRATOR
#ifdef INTEGRATOR
  const byte CH1OS_VAL = 0x80;        
  const byte GAIN_VAL = 0x14;          //this one is the gain (page 14 of the datasheet)
  const float POWER_RATIO =  0.827;   //per manual
#else
  const byte CH1OS_VAL = 0x00;
  const byte GAIN_VAL = 0x00;
  const float POWER_RATIO =  0.848;   //per manual
#endif

const int VMODE = 0x608C;  // sample current on ADC1 at 27.9kps
const int IMODE = 0x408C;

const int WAVE_COUNT_MASK = SAMPLE_DIV-1;   
const int HALF_CYCLES = 120*SECONDS;
const int MAX_SAMPLES = 29000/SAMPLE_DIV*SECONDS;

////////////////////////// Global resourses /////////////////////////////////

//instance of the class that handles the AD chip
AD_Chip AD;  

// the lights
led Led;

// array of samples we use to calculate our wave info
long samples[MAX_SAMPLES];  

////////////////////////////// Main Functions /////////////////////////////

// this function has to be present, otherwise watchdog won't work
void watchdogSetup(void) 
{
// do what you want here
}

// Separted out AD config so we can reconfigure if the chip resets for some reason
void configAD(bool wait = true){
  digitalWrite(RED_LED, LED_ON);
  digitalWrite(AMBER_LED, LED_OFF);
  AD.setup();
  AD.write_int(MODE, IMODE);  
  AD.write_byte(CH1OS, CH1OS_VAL);    // integrator setting
  AD.write_byte(GAIN, GAIN_VAL);      // gain adjust
  
  AD.enable_irq(WSMP);    // New data is present in the waveform register.
  AD.enable_irq(ZX);      // zero crossing interrupt - we have the pin also, but sometimes it makes sense to read it here
  digitalWrite(RED_LED, LED_OFF);
  watchdogReset();
  // if config didn't stick there is a hw issue, so no point in going any further
  if( AD.read_int(MODE) != IMODE ){
    Serial.println(F("Error communicating with AD chip, check daughter card is seated properly!"));
    // freeze here
    while(1);
  }
  if(wait)
  {
    if(Serial)
      Serial.println(F("Waiting 5 seconds to allow stabilization...")); 
    // wait few seconds for for readings to stabilize
    int count = 0;
    digitalWrite(AMBER_LED, LED_ON);
    while( !AD.read_irq() || !AD.irq_set(ZX) || ++count < 600 );
    digitalWrite(AMBER_LED, LED_OFF);
  }
}

//the setup function runs once when reset button pressed, board powered on, or serial connection
void setup() {
  // Enable watchdog timer for 20 seconds
  watchdogEnable(20000);
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(AMBER_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LED_ON);
  digitalWrite(AMBER_LED, LED_ON);
  digitalWrite(RED_LED, LED_ON);
  delay(2000);
  digitalWrite(GREEN_LED, LED_OFF);
  digitalWrite(AMBER_LED, LED_OFF);
  digitalWrite(RED_LED, LED_OFF);
  delay(1000);
  if(Serial)
    Serial.println(F("Booting!")); 
#ifdef WIFI
  configAD(false);
  pass_wifi_values(WLAN_SSID, WLAN_PASS, WLAN_SECURITY, SERVER_ADDRESS);
  while(!mysql_connect());  //loop until we connect to the server
#else
  configAD();
#endif    

  
  
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
  watchdogReset();
  float power_base, max_diff;
  float package[14] = {0};
  if( Serial )
        Serial.print(' ');
  // we average AVE_COUNT signitures together before sending to the server
  for(int outer_i = 0; outer_i < AVE_COUNT ; ++outer_i){
    int next_outer_i = outer_i+1; //used enough we precalculate it
    if( Serial )
        Serial.print(next_outer_i);
    digitalWrite(AMBER_LED, LED_ON);
    // reset our state
    int sample_count = 0;
    int waveform_count = 0;
    int half_cycle_count = 0;
  
    // wait until the next neg->pos zero crossing 
    while( digitalRead(AD_zx) );
    while( !digitalRead(AD_zx) );
    // clear the interrupt status before starting our data grabbing loop
    AD.read_irq(); 
     // and clear the power accumulators
    AD.read_long(RAENERGY); 
    AD.read_long(RVAENERGY);

    
    // our "gather data" loop
    while(1){
      
      int watchdog_count = 0;
      // poll for the next irq
      while( !AD.read_irq() ){
        // watchdog counter to detect a chip reset in which a irq will never set
        if(++watchdog_count > 2000){
          if( Serial )
            Serial.println(F("AD chip reset detected, reconfiguring..."));
          configAD();
          return; 
        }
      }
      
      // if we have a new waveform availible grab it if
      // we set sampling to 29k but we actually use a lower rate (determined in settings)
      // the reason for this is have finer granularity of starting at the zero crossing for better phase info
      // for example if WAVE_COUNT_MASK=7 then our real rate is 29k/8
      if( AD.irq_set(WSMP) && !(++waveform_count & WAVE_COUNT_MASK)){
#ifdef DUMPER
// Dumper we compress the data to 3 bytes so otherwise it takes to long to dump live
// and all we do is constantly dump the waveform over and over
        long data = AD.read_waveform();
        data &= ~(1<<23);             // clear the msb
        data |= digitalRead(AD_zx)<<23;  // put the ZX signal at the msb
        Serial.write((char*)&data, 3); // spit it out - 3 bytes as that is all Serial can handle realtime
      }
#else    
        samples[sample_count++] = AD.read_waveform();
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
    // another check for AD chip reset - sometimes irq's still work but data is garbage until we reconfigure
    if( AD.read_int(MODE) != IMODE){
      if( Serial )
          Serial.println(F("AD chip reset detected, reconfiguring..."));
      configAD();
      return;
    }
    digitalWrite(AMBER_LED, LED_OFF);
    // data collection done. Lets grab our powers
    process_data::N = sample_count;
    process_data::Sample_Rate = sample_count*1.0/SECONDS;
    float active = AD.read_long(RAENERGY) * (POWER_RATIO * POWER_SCALAR / SECONDS) + ACTIVE_ZERO_ADJUST;
    float apparent = AD.read_long(RVAENERGY) * ( POWER_SCALAR / SECONDS) + APPARENT_ZERO_ADJUST;
    float reactive = sqrt(apparent*apparent - active*active);
#ifdef SWEEP
// Sweep mode we spit out all the info to check calibration
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
///////// Normal operation here ////////////
    // just in case
    if( isnan(reactive) )
       reactive = 0;
    // depending on how little the power level is we may need diffe
    if( outer_i == 0 ){
      power_base = active;
      if( power_base < 6)
        max_diff = 2.5;
      if( power_base < 15)
        max_diff = 1.5;
      else
        max_diff = 1;
    }
    // we ensure the power didn't change, if so a device changed
    else if(abs(active-power_base) > max_diff){
      if( Serial ){
        Serial.print(F("Power change of "));
        Serial.print(power_base-active);
        Serial.println(F(" detected, Starting over..."));
      }
      return;
    }
    
#ifdef MAG_PHASE
// displayed every calculation in this mode
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
    if( Serial )
        Serial.print(F(", "));
    if(next_outer_i == AVE_COUNT){
#ifdef WIFI
      if( Serial )
        Serial.println(F("Sending:"));
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

