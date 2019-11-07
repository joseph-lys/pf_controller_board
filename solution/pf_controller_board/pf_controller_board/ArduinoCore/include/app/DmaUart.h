/*
 * dmaUART.h
 *
 * Created: 3/11/2019 2:57:25 PM
 *  Author: Joseph
 */ 


#ifndef DMAUARTMASTER_H_
#define DMAUARTMASTER_H_

#include "XSERCOM.h"
#include "DmaInstance.h"
#include "callback.h"
class DmaUart : public Callback {
  public:
    enum {
      is_busy,
      is_done,
      is_timeout
    };
    DmaUart(XSERCOM *_s, uint8_t _dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX);
    
    // configure UART to specified settings
    void begin(unsigned long baudrate);
    void begin(unsigned long baudrate, uint16_t config);
    
    // write to uart
    void write(uint8_t* _tx_buf, uint32_t _tx_len);
    
    // read from uart
    void read(uint8_t* _tx_buf, uint32_t _tx_len);

    // returns the current state
    int poll();
    
    // stop transfer
    void stopTransfer();
    
    // callback function
    void callback (int) override final;
    
  private:
    XSERCOM* sercom;
    DmaInstance dma_;
    uint8_t dma_channel;
    uint8_t uc_pinRX;
    uint8_t uc_pinTX;
    SercomRXPad uc_padRX;
    SercomUartTXPad uc_padTX;
    
    int current_state;
    void init();
    SercomNumberStopBit extractNbStopBit(uint16_t config);
    SercomUartCharSize extractCharSize(uint16_t config);
    SercomParityMode extractParity(uint16_t config);
};




#endif /* DMAUARTMASTER_H_ */