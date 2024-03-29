#pragma once

#include <stdint.h>
#include <functional>

#define ONEWIRE_SEARCH 1
#define ONEWIRE_CRC 1
#define ONEWIRE_CRC16 1

namespace onewire {


#if ONEWIRE_CRC
    // Compute a Dallas Semiconductor 8 bit CRC, these are used in the
    // ROM and scratchpad registers.
    uint8_t crc8(const uint8_t *addr, uint8_t len);

#if ONEWIRE_CRC16
    // Compute the 1-Wire CRC16 and compare it against the received CRC.
    // Example usage (reading a DS2408):
    //    // Put everything in a buffer so we can compute the CRC easily.
    //    uint8_t buf[13];
    //    buf[0] = 0xF0;    // Read PIO Registers
    //    buf[1] = 0x88;    // LSB address
    //    buf[2] = 0x00;    // MSB address
    //    WriteBytes(net, buf, 3);    // Write 3 cmd bytes
    //    ReadBytes(net, buf+3, 10);  // Read 6 data bytes, 2 0xFF, 2 CRC16
    //    if (!CheckCRC16(buf, 11, &buf[11])) {
    //        // Handle error.
    //    }     
    //          
    // @param input - Array of bytes to checksum.
    // @param len - How many bytes to use.
    // @param inverted_crc - The two CRC16 bytes in the received data.
    //                       This should just point into the received data,
    //                       *not* at a 16-bit integer.
    // @param crc - The crc starting value (optional)
    // @return True, iff the CRC matches.
    bool check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc = 0);

    // Compute a Dallas Semiconductor 16 bit CRC.  This is required to check
    // the integrity of data received from many 1-Wire devices.  Note that the
    // CRC computed here is *not* what you'll get from the 1-Wire network,
    // for two reasons:
    //   1) The CRC is transmitted bitwise inverted.
    //   2) Depending on the endian-ness of your processor, the binary
    //      representation of the two-byte return value may have a different
    //      byte order than the two bytes you get from 1-Wire.
    // @param input - Array of bytes to checksum.
    // @param len - How many bytes to use.
    // @param crc - The crc starting value (optional)
    // @return The CRC16, as defined by Dallas Semiconductor.
    uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);
#endif
#endif


enum PinMode { INPUT, OUTPUT };
enum IntMode { INTR_ON, INTR_OFF };
using SetPinModeClbk = std::function<void(const PinMode)>;
using ReadPinValueClbk = std::function<bool()>;
using SetPinValue = std::function<void(const bool)>;
using SetIntrMode = std::function<void(const IntMode)>;
using DelayMsecClbk = std::function<void(const uint32_t)>;

struct RegisterNumber {
    union {
        std::uint8_t romData[8];
        #pragma pack(push, 1) 
        struct {
            std::uint8_t family_code;
            std::uint8_t id[6];
            std::uint8_t crc;
        };
        #pragma pack(pop)
        std::uint64_t val64;
    };

    std::uint8_t calcCrc() const {
        return crc8(romData, 7);
    }

    bool isCrcValid() const {
        return crc == calcCrc();
    }
};

class OneWire
{
  private:
    SetPinModeClbk m_setPinModeClbk;
    ReadPinValueClbk m_readPinValueClbk;
    SetPinValue m_setPinValueClbk;
    DelayMsecClbk m_delayMsecClbk;
    SetIntrMode m_setIntrModeClbk;

#if ONEWIRE_SEARCH
    // global search state
    unsigned char ROM_NO[8];
    uint8_t LastDiscrepancy;
    uint8_t LastFamilyDiscrepancy;
    bool LastDeviceFlag;
#endif

  public:
    OneWire( SetPinModeClbk setPinModeClbk,
             ReadPinValueClbk readPinValueClbk,
             SetPinValue setPinValueClbk,
             DelayMsecClbk delayMsecClbk,
             SetIntrMode setIntrModeClbk)
    : m_setPinModeClbk(setPinModeClbk)
    , m_readPinValueClbk(readPinValueClbk)
    , m_setPinValueClbk(setPinValueClbk)
    , m_delayMsecClbk(delayMsecClbk)
    , m_setIntrModeClbk(setIntrModeClbk) { begin(); }
    
    void begin();

    // Perform a 1-Wire reset cycle. Returns 1 if a device responds
    // with a presence pulse.  Returns 0 if there is no device or the
    // bus is shorted or otherwise held low for more than 250uS
    bool reset(void);

    // Issue a 1-Wire rom select command, you do the reset first.
    void select(const uint8_t rom[8]);

    // Issue a 1-Wire rom skip command, to address all on bus.
    void skip(void);

    // Write a byte. If 'power' is one then the wire is held high at
    // the end for parasitically powered devices. You are responsible
    // for eventually depowering it by calling depower() or doing
    // another read or write.
    void write(uint8_t v, uint8_t power = 0);

    void write_bytes(const uint8_t *buf, uint16_t count, bool power = 0);

    // Read a byte.
    uint8_t read(void);

    void read_bytes(uint8_t *buf, uint16_t count);

    // Write a bit. The bus is always left powered at the end, see
    // note in write() about that.
    void write_bit(uint8_t v);

    // Read a bit.
    uint8_t read_bit(void);

    // Stop forcing power onto the bus. You only need to do this if
    // you used the 'power' flag to write() or used a write_bit() call
    // and aren't about to do another read or write. You would rather
    // not leave this powered if you don't have to, just in case
    // someone shorts your bus.
    void depower(void);

#if ONEWIRE_SEARCH
    // Clear the search state so that if will start from the beginning again.
    void reset_search();

    // Setup the search to find the device type 'family_code' on the next call
    // to search(*newAddr) if it is present.
    void target_search(uint8_t family_code);

    // Look for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are
    // no devices, or you have already retrieved all of them.  It
    // might be a good idea to check the CRC to make sure you didn't
    // get garbage.  The order is deterministic. You will always get
    // the same devices in the same order.
    bool search(uint8_t *newAddr, bool search_mode = true);
#endif

private:
    inline void noInterrupts() { m_setIntrModeClbk(INTR_OFF); }

    inline void interrupts() { m_setIntrModeClbk(INTR_ON); }
};

} // end of namespace onewire
