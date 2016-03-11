#ifndef SETTINGS_H
#define SETTINGS_H

#include "Arduino.h"
#include "constants.h"
#define PI 3.1415926535897932384626433832795
/////////////////////// Settings ////////////////////////////

//Adjust these to scale values //
const float power_ratio =  0.848;   //supposedly should be .848
const float power_scaler = .32;  
const int   min_power = 10;  


#define WLAN_SSID       "UCInet Mobile Access"        // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_UNSEC

// IP address as and integer
const unsigned long IP = 2850699055;
//enter port no for the server
const int PORTNO = 12021;

#define INTEGRATOR

const float START_PHASE_CORRECTION = -307;

#ifdef INTEGRATOR
  const byte ch1os = 0x80;        
  const byte gain = 0x14;          //this one is the gain (page 14 of the datasheet)
#else
  const byte ch1os = 0x00;
  const byte gain = 0x00;
#endif

#define AD_CLOCK     3580411 // 3580415
#define SAMPLING_DIV   1024  //1024, 512, 256, or 128

const float ZERO_PHASE_TOLERANCE = 0.5;
const float FSAMPLE_RATE = (float)AD_CLOCK/SAMPLING_DIV;
const int SAMPLE_RATE = FSAMPLE_RATE+.5;
const float THRESHOLD = .1;
const int PHASE_THRESHOLD = 3;

const int POWER_FREQ = 60;
const int RECALIBRATE_CYCLES = 50;
const int SEGMENTS_TO_READ = 11;    //total of 11
const int TRACER_MIN = 348;             // 1/10'th a second
const int MAX_SAMPLES = (SEGMENTS_TO_READ+1)*TRACER_MIN;       //  120 cycles at 3.5ksps and 60hz or excees for 2 seconds of samples




//////////////////// End of settings to change /////////////////////
// variables used in code
float CURRENT_SAMPLE_RATE = FSAMPLE_RATE;
float PHASE_ADV_PER_SAMPLE = 21600/FSAMPLE_RATE;
int   INT_SAMPLE_RATE = FSAMPLE_RATE + .05;
long* PAST_DATA;

// constatns
const float SAMPLES_PER_PERIOD   = FSAMPLE_RATE/POWER_FREQ;
const float DFT_CONST = 2*PI/FSAMPLE_RATE;
const int NYQUIST = SAMPLE_RATE/2;

const int PERIODS_SAMPLED      = MAX_SAMPLES/SAMPLES_PER_PERIOD;
const int TIME_BETWEEN_SAMPLES = 1000000/SAMPLE_RATE; //in us

/// trying fixed point stuff ///s
const int BITSPERCYCLE = 16;
const int Q_INT = pow(2,BITSPERCYCLE);
const float Q_FLOAT = Q_INT;
const int BITSPERQUARTER = BITSPERCYCLE-2;
const float A_DFT_CONST = Q_FLOAT/((float)AD_CLOCK/SAMPLING_DIV);

const int lineCyc = 12;  // 6 cycles so there are 10 'segments' per second.  

#if (SAMPLING_DIV == 128)
    const int vMode = 0x608C;
    const int iMode = 0x408C;
#elif (SAMPLING_DIV == 256)
    const int vMode = 0x688C;
    const int iMode = 0x488C;
#elif (SAMPLING_DIV == 512)
    const int vMode = 0x708C;
    const int iMode = 0x508C;
#else
    const int vMode = 0x5c8C; //swap inputs
    const int iMode = 0x588C; //
#endif





#endif //SETTINGS_H

