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
   enum : int {
     is_nothing = 0,
     is_error = -1,
     is_done = 1
     };
   virtual ~Callback(){};
   virtual void callback(int) = 0;
};




#endif /* CALLBACK_H_ */