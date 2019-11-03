/*
 * safeboot.cpp
 *
 * Created: 3/11/2019 7:28:01 PM
 *  Author: Joseph
 */ 

#include "Arduino.h"

int safeboot( void ) {
    // SAFE BOOT INCASE I MESS UP! MUST BE BEFORE ALL INIT ACTIONS!
    const int sys_pb0_pin = 8;
    volatile int count=0;
    pinMode(sys_pb0_pin, INPUT_PULLDOWN);
    while(count < 50) {
      count++;
    }
    count = 0;
    while(count < 1000) {
      if(digitalRead(sys_pb0_pin)) {
        while(1) { count++; };  // infinite loop
      }
      count++;
    }  
}