//
//  Serial.hpp
//  SparkySim
//
//  Created by Jeffrey Thompson on 4/14/16.
//  Copyright © 2016 Jeffrey Thompson. All rights reserved.
//

#ifndef Serial_hpp
#define Serial_hpp

#include <iostream>

template <typename T>
T F(T val){
    return val;
}

class Serial_sim{
public:
    void begin(int){}
    
    operator bool(){
        return true;
    }
    
    template <typename T>
    void print(T val){
        std::cout<<val;
    }
    
    template <typename T>
    void println(T val){
        std::cout<<val<<'\n';
    }
    
    void write(const void* buf, unsigned int len)
    {
        const char* walk = (const char*)buf;
        while( len ){
            std::cout<<*walk;
            ++walk;
        }
    }
    
    template <typename T>
    void write(T val){
        std::cout<<val;
    }
    
};

#endif /* Serial_hpp */
