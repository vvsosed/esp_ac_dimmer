#include "../include/onewire.h"

namespace onewire {

namespace {
    //void pinMode(const uint8_t _pin, const PinMode _mode) { }

    //IO_REG_TYPE PIN_TO_BITMASK(const uint8_t _pin) {
    //    // #TODO
    //    return 0;
    //}

    //IO_REG_TYPE* PIN_TO_BASEREG(const uint8_t _pin) {
    //    // #TODO
    //    return nullptr;
    //}

    //void DIRECT_MODE_INPUT( volatile IO_REG_TYPE *reg, const IO_REG_TYPE mask) { }

    //void DIRECT_MODE_OUTPUT( volatile IO_REG_TYPE *reg, const IO_REG_TYPE mask) { }

    //void DIRECT_WRITE_LOW( volatile IO_REG_TYPE *reg, const IO_REG_TYPE mask) { }

    //void DIRECT_WRITE_HIGH( volatile IO_REG_TYPE *reg, const IO_REG_TYPE mask) { }

    //uint8_t DIRECT_READ( volatile IO_REG_TYPE *reg, const IO_REG_TYPE mask) {
    //    return 0;
    //}
}


void OneWire::begin()
{
	m_setPinModeClbk(INPUT); //pinMode(pin, INPUT);
	//bitmask = PIN_TO_BITMASK(pin);
	//baseReg = PIN_TO_BASEREG(pin);
#if ONEWIRE_SEARCH
	reset_search();
#endif
}


// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
uint8_t OneWire::reset(void)
{
	uint8_t r;
	uint8_t retries = 125;
   noInterrupts();
	m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(reg, mask);
	interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		m_delayMsecClbk(2);
	} while ( !m_readPinValueClbk() );//( !DIRECT_READ(reg, mask));

	noInterrupts();
	m_setPinValueClbk(false); //DIRECT_WRITE_LOW(reg, mask);
	m_setPinModeClbk(OUTPUT); //DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	interrupts();
	m_delayMsecClbk(500); // 480 minimum
	noInterrupts();
	m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	
   //m_delayMsecClbk(70);
   retries = 1000 / 2;
   do {
		if (--retries == 0) return 0;
		m_delayMsecClbk(2);
	} while ( !m_readPinValueClbk() );

   do {
		if (--retries == 0) return 0;
		m_delayMsecClbk(2);
	} while ( m_readPinValueClbk() );

   do {
		if (--retries == 0) return 0;
		m_delayMsecClbk(2);
	} while ( !m_readPinValueClbk() );

	/*r = !m_readPinValueClbk(); //!DIRECT_READ(reg, mask);
   retries = (480 - 70) / 20;
   while(!r && retries-- > 0) {
      m_delayMsecClbk(10);
      r = !m_readPinValueClbk();
   }*/

   interrupts();
	//m_delayMsecClbk(410); //m_delayMsecClbk(410);
	//return r;

   m_delayMsecClbk(2 * retries);
   return 1;
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
void OneWire::write_bit(uint8_t v)
{
	//IO_REG_TYPE mask IO_REG_MASK_ATTR = bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_BASE_ATTR = baseReg;

	if (v & 1) {
		noInterrupts();
		m_setPinValueClbk(false); //DIRECT_WRITE_LOW(reg, mask);
		m_setPinModeClbk(OUTPUT); //DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		m_delayMsecClbk(10);
		m_setPinValueClbk(true); //DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		interrupts();
		m_delayMsecClbk(55);
	} else {
		noInterrupts();
		m_setPinValueClbk(false); //DIRECT_WRITE_LOW(reg, mask);
		m_setPinModeClbk(OUTPUT); //DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		m_delayMsecClbk(65);
		m_setPinValueClbk(true); //DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		interrupts();
		m_delayMsecClbk(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
uint8_t OneWire::read_bit(void)
{
	//IO_REG_TYPE mask IO_REG_MASK_ATTR = bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_BASE_ATTR = baseReg;
	uint8_t r;

	noInterrupts();
	m_setPinModeClbk(OUTPUT); //DIRECT_MODE_OUTPUT(reg, mask);
	m_setPinValueClbk(false); //DIRECT_WRITE_LOW(reg, mask);
	m_delayMsecClbk(3);
	m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	m_delayMsecClbk(10);
	r = m_readPinValueClbk(); //DIRECT_READ(reg, mask);
	interrupts();
	m_delayMsecClbk(53);
	return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void OneWire::write(uint8_t v, uint8_t power /* = 0 */) {
  for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
    OneWire::write_bit( (bitMask & v)? 1 : 0);
  }
  if ( !power) {
	  noInterrupts();
	  m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(baseReg, bitmask);
	  m_setPinValueClbk(false); //DIRECT_WRITE_LOW(baseReg, bitmask);
	  interrupts();
  }
}

void OneWire::write_bytes(const uint8_t *buf, uint16_t count, bool power /* = 0 */) {
  for (uint16_t i = 0 ; i < count ; i++)
    write(buf[i]);
  if (!power) {
    noInterrupts();
    m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(baseReg, bitmask);
    m_setPinValueClbk(false); //DIRECT_WRITE_LOW(baseReg, bitmask);
    interrupts();
  }
}

//
// Read a byte
//
uint8_t OneWire::read() {
    uint8_t bitMask;
    uint8_t r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	if ( OneWire::read_bit()) r |= bitMask;
    }
    return r;
}

void OneWire::read_bytes(uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++)
    buf[i] = read();
}

//
// Do a ROM select
//
void OneWire::select(const uint8_t rom[8])
{
    uint8_t i;

    write(0x55);           // Choose ROM

    for (i = 0; i < 8; i++) write(rom[i]);
}

//
// Do a ROM skip
//
void OneWire::skip()
{
    write(0xCC);           // Skip ROM
}

void OneWire::depower()
{
	noInterrupts();
	m_setPinModeClbk(INPUT); //DIRECT_MODE_INPUT(baseReg, bitmask);
	interrupts();
}

#if ONEWIRE_SEARCH

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void OneWire::reset_search()
{
  // reset the search state
  LastDiscrepancy = 0;
  LastDeviceFlag = false;
  LastFamilyDiscrepancy = 0;
  for(int i = 7; ; i--) {
    ROM_NO[i] = 0;
    if ( i == 0) break;
  }
}

// Setup the search to find the device type 'family_code' on the next call
// to search(*newAddr) if it is present.
//
void OneWire::target_search(uint8_t family_code)
{
   // set the search state to find SearchFamily type devices
   ROM_NO[0] = family_code;
   for (uint8_t i = 1; i < 8; i++)
      ROM_NO[i] = 0;
   LastDiscrepancy = 64;
   LastFamilyDiscrepancy = 0;
   LastDeviceFlag = false;
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
bool OneWire::search(uint8_t *newAddr, bool search_mode /* = true */)
{
   uint8_t id_bit_number;
   uint8_t last_zero, rom_byte_number;
   bool    search_result;
   uint8_t id_bit, cmp_id_bit;

   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = false;

   // if the last call was not the last one
   if (!LastDeviceFlag) {
      // 1-Wire reset
      if (!reset()) {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = false;
         LastFamilyDiscrepancy = 0;
         return false;
      }

      // issue the search command
      if (search_mode == true) {
        write(0xF0);   // NORMAL SEARCH
      } else {
        write(0xEC);   // CONDITIONAL SEARCH
      }

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = read_bit();
         cmp_id_bit = read_bit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1)) {
            break;
         } else {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit) {
               search_direction = id_bit;  // bit write value for search
            } else {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy) {
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               } else {
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);
               }
               // if 0 was picked then record its position in LastZero
               if (search_direction == 0) {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            write_bit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!(id_bit_number < 65)) {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0) {
            LastDeviceFlag = true;
         }
         search_result = true;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0]) {
      LastDiscrepancy = 0;
      LastDeviceFlag = false;
      LastFamilyDiscrepancy = 0;
      search_result = false;
   } else {
      for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
   }
   return search_result;
  }

#endif

#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#if ONEWIRE_CRC8_TABLE
// Dow-CRC using polynomial X^8 + X^5 + X^4 + X^0
// Tiny 2x16 entry CRC table created by Arjen Lentz
// See http://lentz.com.au/blog/calculating-crc-with-a-tiny-32-entry-lookup-table
static const uint8_t PROGMEM dscrc2x16_table[] = {
	0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
	0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
	0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
	0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (Use tiny 2x16 entry CRC table)
uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = *addr++ ^ crc;  // just re-using crc as intermediate
		crc = pgm_read_byte(dscrc2x16_table + (crc & 0x0f)) ^
		pgm_read_byte(dscrc2x16_table + 16 + ((crc >> 4) & 0x0f));
	}

	return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but a little smaller, than the lookup table.
//
uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
#if defined(__AVR__)
		crc = _crc_ibutton_update(crc, *addr++);
#else
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
#endif
	}
	return crc;
}
#endif

#if ONEWIRE_CRC16
bool OneWire::check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc)
{
    crc = ~crc16(input, len, crc);
    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t OneWire::crc16(const uint8_t* input, uint16_t len, uint16_t crc)
{
#if defined(__AVR__)
    for (uint16_t i = 0 ; i < len ; i++) {
        crc = _crc16_update(crc, input[i]);
    }
#else
    static const uint8_t oddparity[16] =
        { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

    for (uint16_t i = 0 ; i < len ; i++) {
      // Even though we're just copying a byte from the input,
      // we'll be doing 16-bit computation with it.
      uint16_t cdata = input[i];
      cdata = (cdata ^ crc) & 0xff;
      crc >>= 8;

      if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
          crc ^= 0xC001;

      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
#endif
    return crc;
}
#endif

#endif

} // end of namespace onewire