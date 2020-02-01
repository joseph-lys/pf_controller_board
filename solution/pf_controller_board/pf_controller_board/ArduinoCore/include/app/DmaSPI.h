/*
 * SPI Master library for Arduino Zero.
 * Copyright (c) 2015 Arduino LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include <Arduino.h>
#include <DmaInstance.h>

// DMA SPI buffer size, must be a power of 2
#define DMA_SPI_BUFFER_SIZE 128

// SPI_HAS_TRANSACTION means SPI has
//   - beginTransaction()
//   - endTransaction()
//   - usingInterrupt()
//   - SPISetting(clock, bitOrder, dataMode)
#define SPI_HAS_TRANSACTION 1

// SPI_HAS_NOTUSINGINTERRUPT means that SPI has notUsingInterrupt() method
#define SPI_HAS_NOTUSINGINTERRUPT 1

#define SPI_MODE0 0x02
#define SPI_MODE1 0x00
#define SPI_MODE2 0x03
#define SPI_MODE3 0x01

#if defined(ARDUINO_ARCH_SAMD)
  // The datasheet specifies a typical SPI SCK period (tSCK) of 42 ns,
  // see "Table 36-48. SPI Timing Characteristics and Requirements",
  // which translates into a maximum SPI clock of 23.8 MHz.
  // Conservatively, the divider is set for a 12 MHz maximum SPI clock.
  #define SPI_MIN_CLOCK_DIVIDER (uint8_t)(1 + ((F_CPU - 1) / 12000000))
#endif


class DmaSPISlaveClass {
  // static_assert(uint32_t(DMA_SPI_BUFFER_SIZE) & uint32_t(DMA_SPI_BUFFER_SIZE - 1) == 0, 
  //              "Expected buffer size to be a power of 2");
  
  public:
  DmaSPISlaveClass(XSERCOM *p_sercom, uint8_t dma_rx_channel, uint8_t dma_tx_channel, uint8_t uc_pinMISO, uint8_t uc_pinSCK, uint8_t uc_pinMOSI, uint8_t uc_pinSS, SercomSpiTXSlavePad, SercomSpiRXSlavePad);
  ~DmaSPISlaveClass();
  enum : uint32_t {
    buffer_size = DMA_SPI_BUFFER_SIZE,
    buffer_mask = DMA_SPI_BUFFER_SIZE - 1
  };
  void begin();
  private:
  const uint32_t _data_register;
  bool initialized;
  volatile bool _tx_pending;
  volatile bool _rx_pending;
  volatile uint8_t* volatile _tw_buffer;
  volatile uint8_t* volatile _tx_buffer;
  uint8_t volatile _w_buffer[DmaSPISlaveClass::buffer_size];
  volatile uint8_t* volatile _rx_buffer;
  volatile uint8_t* volatile _rw_buffer;
  
  uint32_t _prev_rem;
  
  void init();
  void config();
  void end();
  
  XSERCOM *_p_sercom;
  uint8_t _uc_pinMiso;
  uint8_t _uc_pinMosi;
  uint8_t _uc_pinSCK;
  uint8_t _uc_pinSS;

  SercomSpiTXSlavePad _padTx;
  SercomSpiRXSlavePad _padRx;

  DmaInstance dma_rx;
  DmaInstance dma_tx;
    
  public:
  // Call when CS changed
  void startTransactionInterrupt();
  void endTransactionInterrupt();

  // get current TxDataPtr, data will not be queued for transfer until queueTxData is called
  uint8_t* getTxDataPtr();
  
  // flag the current TxDataPtr as ready to transfer
  void queueTxData();
  
  // get current RxDataPtr, if there is no pending RxData, returns a nullptr
  uint8_t* getRxDataPtr();
  
};


extern DmaSPISlaveClass SPI;

#endif
