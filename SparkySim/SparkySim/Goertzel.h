//
//  Goertzel.h
//  WD
//
//  Created by Jeffrey Thompson on 1/29/15.
//  Copyright (c) 2015 CSE142. All rights reserved.
//

#ifndef __WD__Goertzel__
#define __WD__Goertzel__

#include <math.h>
#include "settings.h"

//base class that allows calibrating all clock dependent functions in one function call
class calc
{
public:
    calc()
    {
        if(root)
        {
            calc* walker = root;
            while(walker->next)
                walker = walker->next;
            walker->next = this;
        }
        else
            root = this;
    }
    ~calc()
    {
        if(root == this)
        {
            root = this->next;
            return;
        }
        calc* walker = root;
        while(walker->next != this)
            walker = walker->next;
        walker->next = this->next;
    }
    static void calibrate_all(float sample_rate)
    {
        CURRENT_SAMPLE_RATE = sample_rate;
        INT_SAMPLE_RATE = FSAMPLE_RATE + .05;
        PHASE_ADV_PER_SAMPLE = 21600 / sample_rate;
        calc* walker = root;
        while(walker)
        {
            walker->calculate_constants();
            walker = walker->next;
        }
    }
private:
    virtual void calculate_constants()=0;
    calc* next = 0;
    static calc* root;
};
calc* calc::root = 0;

//storing the partial calculations so they can be done on each sample as they arrive
struct Goerzel
{
    Goerzel() : count(0), q1(0), q2(0)
    {}
    void clear()
    {
        q1 = 0;
        q2 = 0;
        count = 0;
    }
    int count;
    float q1;
    float q2;
};

int phase_from_zero(float a)
{
    if(a > 180)
        return 360-a;
    return a;
}

// the results from a Goerzel algorithm in a nice package
class Goerzel_result
{
public:
    Goerzel_result(float Real = 0, float Imaginary = 0, float PhaseOffset = 0) : real(Real), imaginary(Imaginary)
    {
    	if( PhaseOffset )
    		adjust_phase(PhaseOffset);
    }
    float amplitude()
    {
        return sqrtf(power());
    }
    float power()
    {
        return real * real + imaginary * imaginary;
    }
    float phase()
    {
        float my_phase = atan2f(imaginary, real);
        return my_phase;
    }
    void operator += (const Goerzel_result& other)
    {
        real += other.real;
        imaginary += other.imaginary;
    }
    void operator /= (int divider)
    {
        real /= divider;
        imaginary /= divider;
    }
    void adjust_phase(float adjust_amount)
    {
        float amp = amplitude();
        float phase = atan2f(imaginary, real);
        real = cosf(phase)*amp;
        imaginary = sinf(phase)*amp;
    }
    void clear()
    {
        real = 0;
        imaginary = 0;
    }
    
public:
    float real, imaginary;
};

//std::ostream& operator << (std::ostream& out, const Goerzel_result& gr)
//{
//    out<<gr.amp<<' '<<gr.phase;
//    return out;
//}


// get the amplitude and phase at a set freq and N samples for a dataset
class process_data : public calc
{
public:
    process_data(float Target_Freq, int N) : target_freq(Target_Freq), n(N)
    {
        calculate_constants(N);
    }
    void process(Goerzel& G, long sample)
    {
        float q0 = coef * G.q1 - G.q2 + sample;
        G.q2 = G.q1;
        G.q1 = q0;
        ++G.count;
    }
    Goerzel_result process_all(long* data)
    {
        Goerzel G;
        if (data+n >= PAST_DATA)
        {
            // critical error only we aren't using exceptions so return a empty result
            return Goerzel_result();
        }
        for(int i = 0; i < n; ++i)
            process(G, data[i]);
        return get_results(G);
    }
    Goerzel_result get_results(Goerzel& G)
    {
        Goerzel_result gr;
        float real = (G.q1*a + G.q2*c) / normalizer;
        float imaginary = (G.q1*b+G.q2*d) / normalizer;

        G.clear();
        return Goerzel_result(real, imaginary);
    }
    int N()
    {
        return n;
    }
    virtual void calculate_constants()
    {
        calculate_constants(n);
    }
    void calculate_constants(int N)
    {
        n = N;
        normalizer = n/2;
        const float K = target_freq * n / CURRENT_SAMPLE_RATE;
        const float alpha = 2.0 * PI * K / n;
        const float beta = 2.0 * PI * K * (n-1) / n;
        a = cosf(beta);
        b = -sinf(beta);
        c = sinf(alpha)*sinf(beta)-cosf(alpha)*cosf(beta);
        d = sinf(2*PI*K);
        coef = 2*cosf(alpha);
    }
private:
    int n;
    float target_freq, a, b, c, d, coef, normalizer;
};

// version that adjusts n for to the sample rate
class divider_process : public process_data
{
public:
    divider_process(float Target_Freq, float Divider = 1) : process_data(Target_Freq, (CURRENT_SAMPLE_RATE/Divider) + .5), divider(Divider)
    {
        //parent class does the work
    }
    virtual void calculate_constants()
    {
        process_data::calculate_constants((CURRENT_SAMPLE_RATE/divider) + .5);
    }
private:
    float divider;
};

// faster version that returns power only - useful for calibrating the sample_rate
// also allows float freq input where process_data accepts only int
class power_only
{
public:
    power_only(float Target_Freq)
    {
        target_freq = Target_Freq;
        calculate_constants(CURRENT_SAMPLE_RATE);
    }
    void process(Goerzel& G, long sample)
    {
        float q0 = coef * G.q1 - G.q2 + sample;
        G.q2 = G.q1;
        G.q1 = q0;
        ++G.count;
    }
    float process_all(long* data, int n)
    {
        Goerzel G;
        for(int i = 0; i < n; ++i)
            process(G, data[i]);
        return result(G);
    }
    float result(Goerzel& G)
    {
        return G.q2*G.q2 + G.q1*G.q1 - coef*G.q1*G.q2;
    }
    void calculate_constants(float sample_rate)
    {
        const float omega = 2.0 * PI * target_freq / sample_rate;
        coef = 2*cosf(omega);
    }
private:
    float target_freq, coef;
};


#endif /* defined(__WD__Goertzel__) */
