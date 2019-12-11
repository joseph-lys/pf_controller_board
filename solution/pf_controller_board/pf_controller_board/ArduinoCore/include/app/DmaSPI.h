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

class SPISettings {
  public:
  SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {
    if (__builtin_constant_p(clock)) {
      init_AlwaysInline(clock, bitOrder, dataMode);
    } else {
      init_MightInline(clock, bitOrder, dataMode);
    }
  }

  // Default speed set to 4MHz, SPI mode set to MODE 0 and Bit order set to MSB first.
  SPISettings() { init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0); }

  bool operator==(const SPISettings& rhs) const
  {
    if ((this->clockFreq == rhs.clockFreq) &&
        (this->bitOrder == rhs.bitOrder) &&
        (this->dataMode == rhs.dataMode)) {
      return true;
    }
    return false;
  }

  bool operator!=(const SPISettings& rhs) const
  {
    return !(*this == rhs);
  }

  uint32_t getClockFreq() const {return clockFreq;}
  uint8_t getDataMode() const {return (uint8_t)dataMode;}
  BitOrder getBitOrder() const {return (bitOrder == MSB_FIRST ? MSBFIRST : LSBFIRST);}

  private:
  void init_MightInline(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {
    init_AlwaysInline(clock, bitOrder, dataMode);
  }

  void init_AlwaysInline(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) __attribute__((__always_inline__)) {
    this->clockFreq = (clock >= (F_CPU / SPI_MIN_CLOCK_DIVIDER) ? F_CPU / SPI_MIN_CLOCK_DIVIDER : clock);

    this->bitOrder = (bitOrder == MSBFIRST ? MSB_FIRST : LSB_FIRST);

    switch (dataMode)
    {
      case SPI_MODE0:
        this->dataMode = SERCOM_SPI_MODE_0; break;
      case SPI_MODE1:
        this->dataMode = SERCOM_SPI_MODE_1; break;
      case SPI_MODE2:
        this->dataMode = SERCOM_SPI_MODE_2; break;
      case SPI_MODE3:
        this->dataMode = SERCOM_SPI_MODE_3; break;
      default:
        this->dataMode = SERCOM_SPI_MODE_0; break;
    }
  }

  uint32_t clockFreq;
  SercomSpiClockMode dataMode;
  SercomDataOrder bitOrder;

  friend class DmaSPISlaveClass;
};

const SPISettings DEFAULT_SPI_SETTINGS = SPISettings();


class DmaSPISlaveClass {
  public:
  DmaSPISlaveClass(XSERCOM *p_sercom, uint8_t dma_rx_channel, uint8_t dma_tx_channel, uint8_t uc_pinMISO, uint8_t uc_pinSCK, uint8_t uc_pinMOSI, uint8_t uc_pinSS, SercomSpiTXSlavePad, SercomSpiRXSlavePad);
  ~DmaSPISlaveClass();
  const uint32_t buffer_size;
  void setBitOrder(BitOrder order);
  void setDataMode(uint8_t uc_mode);
  void begin();
  private:
  const uint32_t _data_register;
  bool initialized;
  volatile bool _tx_pending;
  volatile bool _rx_pending;
  volatile uint8_t* volatile _tw_buffer;
  volatile uint8_t* volatile _tx_buffer;
  volatile uint8_t* volatile _w_buffer;
  volatile uint8_t* volatile _rw_buffer;
  
  void init();
  void config(SPISettings settings);
  void end();
  
  XSERCOM *_p_sercom;
  uint8_t _uc_pinMiso;
  uint8_t _uc_pinMosi;
  uint8_t _uc_pinSCK;
  uint8_t _uc_pinSS;

  SercomSpiTXSlavePad _padTx;
  SercomSpiRXSlavePad _padRx;

  SPISettings settings;
  DmaInstance dma_rx;
  DmaInstance dma_tx;
    
  public:
  // this is called from within SERCOMX_Handler
  //   new data goes to pending_rx, 
  //   pending_tx is cleared.
  //   dma channels are stopped.
  void ssInterrupt();

  // get current TxDataPtr, data will not be queued for transfer until queueTxData is called
  uint8_t* getTxDataPtr();
  
  // flag the current TxDataPtr as ready to transfer
  void queueTxData();
  
  // get current RxDataPtr, if there is no pending RxData, returns a nullptr
  uint8_t* getRxDataPtr();
   
};


extern DmaSPISlaveClass SPI;

#endif
