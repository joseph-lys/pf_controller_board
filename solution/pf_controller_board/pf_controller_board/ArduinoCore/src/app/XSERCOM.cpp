/*
 * XSERCOM.cpp
 *
 * Created: 3/11/2019 9:50:32 PM
 *  Author: Joseph
 */ 

#include "XSERCOM.h"



XSERCOM::XSERCOM(Sercom* s) 
: SERCOM(s) {
  
}

uint8_t XSERCOM::getSercomId() {
  uint8_t x = 0;
  if(sercom == SERCOM0) { 
    x = 0; 
  } else if(sercom == SERCOM1) {
    x = 1;
  } else if(sercom == SERCOM2) {
    x = 2;
  } else if(sercom == SERCOM3) {
    x = 3;
  } else if(sercom == SERCOM4) {
    x = 4; 
  } else if(sercom == SERCOM5) {
    x = 5;
  }
  return x;
}

Sercom* XSERCOM::getSercomPointer() {
  return sercom;
}