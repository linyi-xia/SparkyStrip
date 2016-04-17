#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include "settings.h"

/*
class calculations{
public:
  calculations(){
    reset();
  }
  void reset(){
    
  }
private:
  long _max, _min, _rise_time,
};
*/

//3796us per sample for uno
//1446us per sample for DUE

// Returns the max of the buffer
long getMaxValue(long* buffer, int buffer_size)
{
  long max = -20000;
  
  for(int count = 0; count < buffer_size; count++) {
    if (max < buffer[count])
    {
        max = buffer[count];
    }
}
  return max;
}
// Returns the Min of the buffer
long getMinValue(long* buffer, int buffer_size)
{
  long min = 20000;
  
  for(int count = 0; count < buffer_size; count++) {
    if (min > buffer[count])
    {
        min = buffer[count];
    }
  }
return min;
}

// Returns the rise time
long getRiseTime(long* buffer, int buffer_size)
{
  int rise_time = 0;
  bool flag = true;
  int max_at_period;
  int counter = 0;
  
// Finds the "rise time" (number of samples) for the number of periods desired, and finds the average
  for(int i = 1; i < PERIODS_SAMPLED; ++i)
  {
    max_at_period = 0;
// 2nd, 3rd and 4th Period, depending on for_loop.  Finds the max at loop, and stores the max and buffer location
    int top = (i+1)*SAMPLES_PER_PERIOD;
    for(int count = (i)*SAMPLES_PER_PERIOD; count < top; count++)
    {
      if (max_at_period < buffer[count])
      {
          max_at_period = buffer[count];
          counter = count;
      }
    }
    
    flag = true;
//Finds the "array index" at 90% value of max in second wave
    while(flag)
    {
      if( buffer[counter] < 0.9 * max_at_period)
      {
        flag = false;
      }
      else
      {
        counter--;
      }
    }
    flag = true;
    while(flag)
    {
      if( buffer[counter] < 0.1 * max_at_period)
      {
        flag = false;
      }
      counter--;
      rise_time++;
    }
  }
  
  return rise_time*TIME_BETWEEN_SAMPLES/PERIODS_SAMPLED;
}

void a_dft(long* buffer, float freq, float &amplitude, float &phase){
  
  const int phase_step = freq*A_DFT_CONST;
  int wave_phase = 0; //phase of target freq
  float real = buffer[0];
  float imaginary = 0;
    
  for(int count=1; count<MAX_SAMPLES; ++count)
  {
    wave_phase += phase_step;
    
    float sinout, cosout;
    
    // Modulo phase into quarter, convert to float 0..1
    float modphase = (wave_phase & (1<<BITSPERQUARTER)-1)
        *1.0f/(1<<BITSPERQUARTER);
    // Extract quarter bits 
    int quarter = wave_phase & (3<<BITSPERQUARTER);
    // Recognize quarter
    if (!quarter) { 
        // First quarter, angle = 0 .. pi/2
        float x = modphase - 0.5f;      // 1 sub
        float temp = .75 - x*x;         // 1 mul, 1 sub
        sinout = temp + x;              // 1 add
        cosout = temp - x;              // 1 sub
    } else if (quarter == 1<<BITSPERQUARTER) {
        // Second quarter, angle = pi/2 .. pi
        float x = 0.5f - modphase;      // 1 sub
        float temp = x*x + .75;         // 1 mul, 1 add
        sinout = x + temp;              // 1 add
        cosout = x - temp;              // 1 sub
    } else if (quarter == 2<<BITSPERQUARTER) {
        // Third quarter, angle = pi .. 1.5pi
        float x = modphase - 0.5f;      // 1 sub
        float temp = x*x - .75;         // 1 mul, 1 sub
        sinout = temp - x;              // 1 sub
        cosout = temp + x;              // 1 add
    } else {
        // Fourth quarter, angle = 1.5pi..2pi
        float x = modphase - 0.5f;      // 1 sub
        float temp = .75 - x*x;         // 1 mul, 1 sub
        sinout = x - temp;              // 1 sub
        cosout = x + temp;              // 1 add
    }
    real += buffer[count]*sinout;   //only real data so dropping imaginary terms on these
    imaginary -= buffer[count]*cosout;
  }
  // normalize so changing number of samples doesn't change scale
  real/=MAX_SAMPLES;
  imaginary/=MAX_SAMPLES;
  amplitude = sqrtf(real*real + imaginary*imaginary);
  phase = atanf(imaginary/real)*180/PI;

}

void dft(long* buffer, float freq, float &amplitude, float &phase){
  
  const float phase_step = freq*DFT_CONST;
  float wave_phase = 0; //phase of target freq
    //for first sample our wavephase is 0, so the real component is buffer[0]*1 as cos(0) is 1 and sin(0) is 0;
  float real = buffer[0];
  float imaginary = 0;
    
  for(int count=1; count<MAX_SAMPLES; ++count)
  {
    wave_phase += phase_step;
    real += buffer[count]*cosf(wave_phase);   //only real data so dropping imaginary terms on these
    imaginary -= buffer[count]*sinf(wave_phase);
  }
  // normalize so changing number of samples doesn't change scale
  real/=MAX_SAMPLES;
  imaginary/=MAX_SAMPLES;
  amplitude = sqrtf(real*real + imaginary*imaginary);
  phase = atanf(imaginary/real)*180/PI;
}

// FFT at 180 Hertz.  Assumes 5000Hz Sampling POWER_FREQ
float dft180(long* buffer, int buffer_size)
{
    // Initialize
    float real_value = 0.0;
    float imaginary_value = 0.0;
    float FFT_60Hz = 0.0;
    float FFT_180Hz = 0.0;
    float FFT_Val = 0.0;
	
    // Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
    // Because the 60 Hz FFT may be represented in a different buffer, we check values around it for accuracy
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(5+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(5+i)/buffer_size));
      }
	
      // 60 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_60Hz)
        FFT_60Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
// Finds the 180 Hertz
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(17+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(17+i)/buffer_size));
      }
	
      // 180 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_180Hz)
        FFT_180Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    //
     
    return FFT_180Hz/FFT_60Hz;
}

float dft300(long* buffer, int buffer_size)
{
    // Initialize
    float real_value = 0.0;
    float imaginary_value = 0.0;
    float FFT_60Hz = 0.0;
    float FFT_300Hz = 0.0;
    float FFT_Val = 0.0;
	
    // Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
    // Because the 60 Hz FFT may be represented in a different buffer, we check values around it for accuracy
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(5+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(5+i)/buffer_size));
      }
	
      // 60 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_60Hz)
        FFT_60Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    // Finds the 300 Hertz
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(30+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(30+i)/buffer_size));
      }
	
      // 180 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_300Hz)
        FFT_300Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
   return FFT_300Hz/FFT_60Hz;
}


#endif // CALCULATIONS_H
