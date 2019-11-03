/*
 * dmaUART.h
 *
 * Created: 3/11/2019 2:57:25 PM
 *  Author: Joseph
 */ 


#ifndef DMAUARTMASTER_H_
#define DMAUARTMASTER_H_

#include "SERCOM.h"

class DmaUartMaster {
  public:
    enum {
      is_busy,
      is_done,
      is_timeout
    };
    DmaUartMaster(Sercom *_s, uint32_t _dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX);
    void begin(unsigned long baudRate);
    void begin(unsigned long baudrate, uint16_t config);
    
    // transmit only
    void transfer(uint8_t* _tx_buf, uint32_t _tx_len, unsigned long int timeout);
    
    // transmit and receive
    void transfer(uint8_t* _tx_buf, uint32_t _tx_len, uint8_t* _rx_buf, uint32_t _rx_len, unsigned long int _timeout);
    
    // returns the current state
    int poll();
    
  private:
    Sercom *sercom;
    SERCOM arduino_sercom;

    uint32_t dma_channel;
    uint8_t uc_pinRX;
    uint8_t uc_pinTX;
    SercomRXPad uc_padRX;
    SercomUartTXPad uc_padTX;
    
    unsigned long int last_transfer;
    unsigned long int timeout;
    int current_state;
    void setup_descriptors();
    SercomNumberStopBit extractNbStopBit(uint16_t config);
    SercomUartCharSize extractCharSize(uint16_t config);
    SercomParityMode extractParity(uint16_t config);
    
    // DmacDescriptors must be 16 bit aligned
    __attribute__((__aligned__(16)))
    DmacDescriptor tx_desc;
    DmacDescriptor rx_desc;
    
};




#endif /* DMAUARTMASTER_H_ */