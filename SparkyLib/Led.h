//
//  Led.h
//  
//
//  Created by Jeffrey Thompson on 5/10/16.
//
//

#ifndef Led_h
#define Led_h

#define GREEN 48
#define AMBER 50
#define RED   52

class led{
public:
    led(){
        pinMode(GREEN, OUTPUT);
        pinMode(AMBER, OUTPUT);
        pinMode(RED, OUTPUT);
    }
    void on(uint64_t led_ID){
        digitalWrite(led_ID, HIGH);
    }
    void off(uint64_t led_ID){
        digitalWrite(led_ID, LOW);
    }
};

// the instance of the class
led Led;


#endif /* Led_h */
