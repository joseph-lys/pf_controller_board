/*
 * callback.h
 *
 * Created: 3/11/2019 11:57:21 PM
 *  Author: Joseph
 */ 


#ifndef CALLBACK_H_
#define CALLBACK_H_

class Callback {
  public:
   virtual ~Callback() { };
   virtual void callback(int) = 0;
};




#endif /* CALLBACK_H_ */