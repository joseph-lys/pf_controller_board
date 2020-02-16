/*
 * function_wrapper.h
 *
 * Wraps a function into a common function type 
 * so that either member or non-member function can be used.
 * 
 *
 * function has the signature: 
 *   uint32_t functionName (uint32_t, uint32_t);
 * 
 * 
 * Created: 15/2/2020 12:57:24 PM
 *  Author: joseph
 */ 


#ifndef CALLBACK_H_
#define CALLBACK_H_

class Func_ {
  public:
  Func_(){}
  virtual ~Func_(){}
  virtual uint32_t operator () (uint32_t x, uint32_t y) { return 0; }
};

template <class T>
class MemFunc_ : public Func_ {
  private:
  typedef uint32_t (T::*func_type)(uint32_t, uint32_t);
  T* ins_;
  func_type func_;
  public:
  MemFunc_() = delete;
  MemFunc_(T& ins, func_type func): ins_(&ins), func_(func) {}
  uint32_t operator () (uint32_t x, uint32_t y) final {
    return (ins_->*func_)(x, y);
  }
};

class NonMemFunc_ : public Func_ {
  private:
  typedef uint32_t (*func_type)(uint32_t, uint32_t);
  func_type func_;
  public:
  NonMemFunc_() = delete;
  NonMemFunc_(func_type func): func_(func) {}
  uint32_t operator() (uint32_t x, uint32_t y) final {
    return (*func_)(x, y);
  }
};

class Callback {
  Func_* f_;
  int* ref_count_;
  void reduceRef() {
    if(ref_count_) {
      if((*ref_count_) <= 1) {
        if(f_) {
          delete f_;
          f_ = nullptr;
        }
        delete ref_count_;
        ref_count_ = nullptr;
        } else {
        (*ref_count_)--;
      }
    }
  }
  void copyRef(Callback& other) {
    ref_count_ = other.ref_count_;
    f_ = other.f_;
    (*ref_count_)++;
  }
  
  void moveRef(Callback& other) {
    ref_count_ = other.ref_count_;
    f_ = other.f_;
    other.ref_count_ = nullptr;
    other.f_ = nullptr;
  }
  public:
  Callback(): f_(nullptr), ref_count_(new int{1}) { }  // default constructor
  
  template<typename T>
  Callback(T& ins, uint32_t (T::*func)(uint32_t, uint32_t)) : Callback() {  // constructor for member function
    f_ = new MemFunc_<T>{ins, func};
  }
  
  Callback(uint32_t (*func)(uint32_t, uint32_t)) : Callback() {  // constructor for non-member function
    f_ = new NonMemFunc_{func};
  }
  
  ~Callback() {
    reduceRef();
  }
  
  Callback& operator=(Callback& other) {  // copy operator
    reduceRef();
    copyRef(other);
  };
  Callback& operator=(Callback&& other) {  // move operator
    reduceRef();
    moveRef(other);
  };
  Callback(Callback& other) : f_(nullptr), ref_count_(nullptr) {  // copy constructor
    copyRef(other);
  };
  Callback(Callback&& other) : f_(nullptr), ref_count_(nullptr) {  // move constructor
    moveRef(other);
  };
  /// check if it is a valid callback
  bool isValid() {
    if(f_) {
      return true;
    }
    return false;
  }
  /// call function
  uint32_t operator()(uint32_t x, uint32_t y) {
    if(f_) {
      (*f_)(x, y);
    }
  }
};


#endif /* CALLBACK_H_ */