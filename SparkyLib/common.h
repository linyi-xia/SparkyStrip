//
//  common.h
//  
//
//  Created by Jeffrey Thompson on 5/10/16.
//
//

#ifndef common_h
#define common_h

#define GREEN_LED 48
#define AMBER_LED 50
#define RED_LED   52
#define LED_ON HIGH
#define LED_OFF LOW

void go_error(int ms_flash){
    bool status = true;
    while(1)
    {
        status^=true;
        if(status)
            digitalWrite(RED_LED, LED_ON);
        else
            digitalWrite(RED_LED, LED_OFF);
        delay(ms_flash);
    }
}


#endif /* common_h */
