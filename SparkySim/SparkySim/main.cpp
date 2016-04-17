//
//  main.cpp
//  SparkySim
//
//  Created by Jeffrey Thompson on 4/14/16.
//  Copyright © 2016 Jeffrey Thompson. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "SparkySim.ino"



int main(int argc, const char * argv[]) {
    // insert code here...
    if(argc < 2){
        printf("Please pass the filename of the data to process\nExiting.\n");
        return 1;
    }
    FILE* in_file = fopen(argv[1],"r");
    if(!in_file){
        printf("Cannot open the file %s\nExiting.\n",argv[1]);
        return 2;
    }
    AD.setFile(in_file);
    setup();
    while(1)
        loop();
}