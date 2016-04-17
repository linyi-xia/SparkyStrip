//
//  Goertzel.h
//
//  Created by Jeffrey Thompson on 1/29/15.
//  Copyright (c) 2015 CSE142. All rights reserved.
//

#ifndef __Goertzel__
#define __Goertzel__

#include <math.h>


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
        return atan2f(imaginary, real);
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



// get the amplitude and phase at a set freq and N samples for a dataset
class process_data
{
public:
    process_data(float Target_Freq) : target_freq(Target_Freq)
    {
        normalizer = N/2.0;
        const float K = target_freq * N / Sample_Rate;
        const float alpha = 2.0 * PI * K / N;
        const float beta = 2.0 * PI * K * (N-1) / N;
        a = cosf(beta);
        b = -sinf(beta);
        c = sinf(alpha)*sinf(beta)-cosf(alpha)*cosf(beta);
        d = sinf(2*PI*K);
        coef = 2*cosf(alpha);
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
        for(int i = 0; i < N; ++i)
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
static int N;
static int Sample_Rate;
private:
    float target_freq, a, b, c, d, coef, normalizer;
};

int process_data::N = 1;
int process_data::Sample_Rate = 1;


#endif /* defined(__Goertzel__) */
