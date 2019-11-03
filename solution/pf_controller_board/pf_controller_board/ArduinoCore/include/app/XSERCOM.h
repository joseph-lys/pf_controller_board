/*
 * XSERCOM.h
 *
 * Created: 3/11/2019 9:47:54 PM
 *  Author: Joseph
 */ 


#ifndef XSERCOM_H_
#define XSERCOM_H_

#include "SERCOM.h"

class XSERCOM : public SERCOM {
public:
  XSERCOM(Sercom* s);
  uint8_t getSercomId();
  Sercom* getSercomPointer();
};




#endif /* XSERCOM_H_ */