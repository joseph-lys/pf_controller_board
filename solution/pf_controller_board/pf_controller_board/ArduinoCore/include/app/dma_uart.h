/*
 * dmaUART.h
 *
 * Created: 3/11/2019 2:57:25 PM
 *  Author: Joseph
 */ 


#ifndef DMAUARTMASTER_H_
#define DMAUARTMASTER_H_

#include "dma_sercom.h"
#include "dma_instance.h"
#include "callback.h"

// use DMA to continuously read data
class DmaContinuousReader{
 public:
  explicit DmaContinuousReader(uint8_t _dma_channel, XSERCOM* _sercom);
  uint32_t callback(uint32_t, uint32_t);
  int readByte();
  uint32_t available();
  void start();
  void stop();
 protected:
  enum : uint32_t {
    block_shift = 5, // 32 bytes per block
    block_data_size = uint32_t(1) << block_shift,
    block_data_mask = block_data_size - 1,
    num_blocks = 4, // 4 blocks
    num_blocks_mask = num_blocks - 1,
    buffer_size = block_data_size * num_blocks,
    buffer_index_mask = buffer_size - 1
  };
  DmacDescriptor descriptors[num_blocks - 1]  __attribute__((__aligned__(16)));
  DmaInstance dma_;
  XSERCOM* sercom;
  volatile uint32_t read_; // number of bytes read
  volatile uint32_t write_;  // number of bytes written
  volatile uint32_t work_idx_;  // working buffer index
  volatile uint8_t buffer_[buffer_size];
  const uint8_t dma_channel;
  inline uint32_t getWrittenCount();
  void setup();
};

// Use DMA to transmit a single transaction
class DmaOneOffWriter {
 public:
  explicit DmaOneOffWriter(uint8_t _dma_channel, XSERCOM* _sercom);
  Callback post_transfer_callback;
  // begin a single write transaction
  void write(uint8_t* _tx_buf, uint32_t _tx_len);
  
  // terminate any active transactions
  void stopTransfer();
  
  // check if transfer is busy
  bool isBusy();
  
  uint32_t callback(uint32_t, uint32_t);
  
 protected:
  DmaInstance dma_;
  XSERCOM* sercom;
  const uint8_t dma_channel;
  bool is_busy;
};   

class DmaUart {
 public:
  DmaUart(XSERCOM *_s, uint8_t _rx_dma_channel, uint8_t _tx_dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX);
  DmaContinuousReader reader;
  DmaOneOffWriter writer;
  
  // configure UART to specified settings
  void begin(unsigned long baudrate);
  void begin(unsigned long baudrate, uint16_t config);
    
  // write to uart
  void write(uint8_t* _tx_buf, uint32_t _tx_len);

  // transmission is in progress
  bool isTransmitting();
    
  // terminate any existing transmits
  void stopTransmit();
  
  int available();
  
  int read();
        
 private:
  XSERCOM* sercom;
  uint8_t uc_pinRX;
  uint8_t uc_pinTX;
  SercomRXPad uc_padRX;
  SercomUartTXPad uc_padTX;    
   
  void init();
  SercomNumberStopBit extractNbStopBit(uint16_t config);
  SercomUartCharSize extractCharSize(uint16_t config);
  SercomParityMode extractParity(uint16_t config);
};





#endif /* DMAUARTMASTER_H_ */