//
//  AD_Sim.hpp
//  SparkySim
//
//  Created by Jeffrey Thompson on 4/14/16.
//  Copyright © 2016 Jeffrey Thompson. All rights reserved.
//

#ifndef AD_Sim_hpp
#define AD_Sim_hpp

#include "settings.h"

#include <stdio.h>
#include <stdlib.h>

class AD_Chip{
public:
    void setFile(FILE* open_file);
    void setup();

    long read_waveform();

    long read_long(byte reg);  //used for getting the waveform or other 24 bit registers (sign extended if register signed)
    int read_int(byte reg){return 0;}    //used for getting 12-16 byte registers
    byte read_byte(byte reg){return 0;}   //used for getting 6-8 byte registers
    void write_int(byte reg, int data){}    //used for writing 12-16 byte registers
    void write_byte(byte reg, byte data){}   //used for writing 6-8 byte registers

    bool read_irq();            // reads the interrupt pin and if set reads the interrupt register and returns true
    bool irq_set(byte irq);     // returns true if the interrupt asked for is set
    void disable_all_irq();     // clears them all
    void enable_irq(int irq);  // enable one interrupt
    void disable_irq(int irq); // disable one interrupt
    
    int irq_status;
    
    FILE* in_file;
private:
    long active, apparent;
    long waveform;
    int irq_mask;
};

void AD_Chip::setFile(FILE* open_file){
    in_file = open_file;
    fscanf(in_file, "%ld,%ld", &active, &apparent);
    if(feof(stdin))
    {
        printf("File does not have proper format!");
        exit(3);
    }
}


void AD_Chip::setup(){
    irq_mask = 0;
    waveform = 0;
    irq_status = 0;
}

long AD_Chip::read_waveform(){
    return waveform;
}

long AD_Chip::read_long(byte reg){
    switch(reg){
        case LAENERGY: return active;
        case LVAENERGY: return apparent;
        default: return 0;
    }
}


bool AD_Chip::read_irq(){
    byte a,b,c;
    if( fscanf(in_file,"%hhd,%hhd,%hhd,%ld",&a,&b,&c,&waveform) != 4){
        printf("End of File reached!\n");
        exit(0);
    }
    irq_status = irq_mask&((a<<4)|(b<<3)|(c<<2));
    return true;
}

bool AD_Chip::irq_set(byte irq){
    return irq_status & 1<<irq;
}

void AD_Chip::disable_all_irq(){
    irq_mask = 0;
}

void AD_Chip::enable_irq(int irq){
    irq_mask |= (1<<irq);
}

void AD_Chip::disable_irq(int irq){
    irq_mask &= ~(1<<irq);
}




#endif /* AD_Sim_hpp */
