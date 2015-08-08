/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s): Damon Hart-Davis 2015
*/

/*Unit test routines for library code.
 */

// Arduino libraries imported here (even for use in other .cpp files).
//#include <SPI.h>

#define UNIT_TESTS

// Include the library under test.
#include <OTV0p2Base.h>
#include <OTRadioLink.h>
#include <OTRFM23BLink.h>


#if F_CPU == 1000000 // 1MHz CPU indicates V0p2 board.
#define ON_V0P2_BOARD
#endif


void setup()
  {
#ifdef ON_V0P2_BOARD
  // initialize serial communications at 4800 bps for typical use with V0p2 board.
  Serial.begin(4800);
#else
  // initialize serial communications at 9600 bps for typical use with (eg) Arduino UNO.
  Serial.begin(9600);
#endif
  }



// Error exit from failed unit test, one int parameter and the failing line number to print...
// Expects to terminate like panic() with flashing light can be detected by eye or in hardware if required.
static void error(int err, int line)
  {
  for( ; ; )
    {
    Serial.print(F("***Test FAILED*** val="));
    Serial.print(err, DEC);
    Serial.print(F(" =0x"));
    Serial.print(err, HEX);
    if(0 != line)
      {
      Serial.print(F(" at line "));
      Serial.print(line);
      }
    Serial.println();
//    LED_HEATCALL_ON();
//    tinyPause();
//    LED_HEATCALL_OFF();
//    sleepLowPowerMs(1000);
    delay(1000);
    }
  }

// Deal with common equality test.
static inline void errorIfNotEqual(int expected, int actual, int line) { if(expected != actual) { error(actual, line); } }
// Allowing a delta.
static inline void errorIfNotEqual(int expected, int actual, int delta, int line) { if(abs(expected - actual) > delta) { error(actual, line); } }

// Test expression and bucket out with error if false, else continue, including line number.
// Macros allow __LINE__ to work correctly.
#define AssertIsTrueWithErr(x, err) { if(!(x)) { error((err), __LINE__); } }
#define AssertIsTrue(x) AssertIsTrueWithErr((x), 0)
#define AssertIsEqual(expected, x) { errorIfNotEqual((expected), (x), __LINE__); }
#define AssertIsEqualWithDelta(expected, x, delta) { errorIfNotEqual((expected), (x), (delta), __LINE__); }


// Check that correct version of this library is under test.
static void testLibVersion()
  {
  Serial.println("LibVersion");
#if !(0 == ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR) || !(8 == ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR)
#error Wrong library version!
#endif
//  AssertIsEqual(0, ARDUINO_LIB_OTRADIOLINK_VERSION_MAJOR);
//  AssertIsEqual(2, ARDUINO_LIB_OTRADIOLINK_VERSION_MINOR);
  }

// Check that correct versions of underlying libraries are in use.
static void testLibVersions()
  {
  Serial.println("LibVersions");
#if !(0 == ARDUINO_LIB_OTV0P2BASE_VERSION_MAJOR) || !(8 == ARDUINO_LIB_OTV0P2BASE_VERSION_MINOR)
#error Wrong library version!
#endif  
#if !(0 == ARDUINO_LIB_OTRFM23BLINK_VERSION_MAJOR) || !(8 == ARDUINO_LIB_OTRFM23BLINK_VERSION_MINOR)
#error Wrong library version!
#endif
  }

// Test class for printing stuff.
class PrintToBuf : public Print
  {
  private:
    int count;
  public:
    PrintToBuf(uint8_t *bufp, size_t bufl) : count(0), buf(bufp), buflen(bufl) { }
    virtual size_t write(const uint8_t b)
      {
      if(count < buflen)
        {
        buf[count] = b;
        // If there is space, null terminate.
        if(count+1 < buflen) { buf[count+1] = '\0'; }
        }
      ++count;
      }
    int getCount() { return(count); }
    uint8_t *const buf;
    const uint8_t buflen;
  };

// Test the frame-dump routine.
static void testFrameDump()
  {
  Serial.println("FrameDump");
  const char *s1 = "Hello, world!";
  // Eyeball output...
  OTRadioLink::dumpRXMsg((uint8_t *)s1, strlen(s1));
  // Real test...
  uint8_t buf[64];
  PrintToBuf ptb1(buf, sizeof(buf));
  OTRadioLink::printRXMsg(&ptb1, (uint8_t *)s1, strlen(s1));
  AssertIsEqual(32, ptb1.getCount());
  AssertIsEqual(0, strncmp("|13  H e l l o ,   w o r l d !\r\n", (const char *)ptb1.buf, sizeof(buf)));
  PrintToBuf ptb2(buf, sizeof(buf));
  uint8_t m2[] = { '{', '}'|0x80 };
  OTRadioLink::dumpRXMsg(m2, sizeof(m2));
  OTRadioLink::printRXMsg(&ptb2, m2, sizeof(m2));
  AssertIsEqual(0, strncmp("|2  {FD\r\n", (const char *)ptb2.buf, sizeof(buf)));
  PrintToBuf ptb3(buf, sizeof(buf));
  uint8_t m3[] = { 0, 1, 10, 15, 16, 31 };
  OTRadioLink::dumpRXMsg(m3, sizeof(m3));
  OTRadioLink::printRXMsg(&ptb3, m3, sizeof(m3));
  AssertIsEqual(0, strncmp("|6 00010A0F101F\r\n", (const char *)ptb3.buf, sizeof(buf)));
  }

// Do some basic testing of CRC 7/5B routine.
static void testCRC7_5B()
  {
  Serial.println("CRC7_5B");
  // Test the 7-bit CRC (0x5b) routine at a few points.
  const uint8_t crc0 = OTRadioLink::crc7_5B_update(0, 0); // Minimal stats payload with normal power and minimum temperature.
  AssertIsTrueWithErr((0 == crc0), crc0); 
  const uint8_t crc1 = OTRadioLink::crc7_5B_update(0x40, 0); // Minimal stats payload with normal power and minimum temperature.
  AssertIsTrueWithErr((0x1a == crc1), crc1); 
  const uint8_t crc2 = OTRadioLink::crc7_5B_update(0x50, 40); // Minimal stats payload with low power and 20C temperature.
  AssertIsTrueWithErr((0x7b == crc2), crc2); 
  AssertIsEqual(0x7b, OTRadioLink::crc7_5B_update_nz_final(0x50, 40)); 
  }


// Do some basic exercise of the RFM23B class, eg that it compiles.
static void testRFM23B()
  {
  Serial.println("RFM23B");
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS> l0;
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS, -1> l1;
  OTRFM23BLink::OTRFM23BLink<OTV0P2BASE::V0p2_PIN_SPI_nSS, 9> l2;
//#ifdef ON_V0P2_BOARD
//  // Can't do anything with this unless on V0p2 board.
//  l0.preinit(NULL); // Must not break anything nor stall!
//#endif
  }

// Some tests for all ISRRXQueue implementations.
// Assumes being passed an empty freshly-created instance.
// Leaves the queue instance empty when done.
static void allISRRXQueue(OTRadioLink::ISRRXQueue &q)
  {
  uint8_t queueRXMsgsMin;
  uint8_t maxRXMsgLen;
  q.getRXCapacity(queueRXMsgsMin,maxRXMsgLen);
  AssertIsTrueWithErr((queueRXMsgsMin >= 1), queueRXMsgsMin); 
  AssertIsTrueWithErr((maxRXMsgLen >= 64), maxRXMsgLen); 
  AssertIsEqual(0, q.getRXMsgsQueued());
  uint8_t buf1[1];
  AssertIsEqual(0, q.getRXMsg(buf1, 1));
  // Pretend to be an ISR and try to load up a message.
  volatile uint8_t *ib1 = q._getRXBufForInbound();
  AssertIsTrue(NULL != ib1); 
  const uint8_t r1 = OTV0P2BASE::randRNG8();
  *ib1 = r1;
  q._loadedBuf(1);
  // Try to retrieve the queued message.
  AssertIsEqual(1, q.getRXMsgsQueued());
  AssertIsEqual(1, q.getRXMsg(buf1, 1));
  AssertIsEqual(r1, buf1[0]);
  // Check that the queue is empty again.
  AssertIsEqual(0, q.getRXMsgsQueued());
  AssertIsEqual(0, q.getRXMsg(buf1, 1));
  }

// Do some basic exercise of ISRRXQueue1Deep.
static void testISRRXQueue1Deep()
  {
  Serial.println("ISRRXQueue1Deep");
  OTRadioLink::ISRRXQueue1Deep<> q;
  allISRRXQueue(q);
  // Specific tests for this queue type.
  // Check queue is empty again.
  AssertIsEqual(0, q.getRXMsgsQueued());
  uint8_t buf2[2];
  AssertIsEqual(0, q.getRXMsg(buf2, 2));
  // Pretend to be an ISR and try to load up a message.
  volatile uint8_t *ib2 = q._getRXBufForInbound();
  AssertIsTrue(NULL != ib2); 
  const uint8_t r1 = OTV0P2BASE::randRNG8();
  ib2[0] = r1; ib2[1] = 0;
  q._loadedBuf(2);
  // Check that the message was queued.
  AssertIsEqual(1, q.getRXMsgsQueued());
  // Verify that the queue is now full (no space for new RX).
  AssertIsTrue(NULL == q._getRXBufForInbound());
  // Try to retrieve the queued message.
  AssertIsEqual(1, q.getRXMsgsQueued());
  AssertIsEqual(2, q.getRXMsg(buf2, 2));
  AssertIsEqual(r1, buf2[0]);
  // Check that the queue is empty again.
  AssertIsEqual(0, q.getRXMsgsQueued());
  AssertIsEqual(0, q.getRXMsg(buf2, 2));
  }


// BASE
// Test for expected behaviour of RNG8 PRNG starting from a known state.
static void testRNG8()
  {
  Serial.println("RNG8");
  // Reset to known state; API not normally exposed and only exists for unit tests.
  OTV0P2BASE::resetRNG8();
  // Extract and check a few initial values.
  const uint8_t v1 = OTV0P2BASE::randRNG8();
  const uint8_t v2 = OTV0P2BASE::randRNG8();
  const uint8_t v3 = OTV0P2BASE::randRNG8();
  const uint8_t v4 = OTV0P2BASE::randRNG8();
  AssertIsTrue(1 == v1);
  AssertIsTrue(0 == v2);
  AssertIsTrue(3 == v3);
  AssertIsTrue(14 == v4);
  }



// To be called from loop() instead of main code when running unit tests.
// Tests generally flag an error and stop the test cycle with a call to panic() or error().
void loop()
  {
  static int loopCount = 0;

  // Allow the terminal console to be brought up.
  for(int i = 3; i > 0; --i)
    {
    Serial.print(F("Tests starting... "));
    Serial.print(i);
    Serial.println();
    delay(1000);
    }
  Serial.println();


  // Run the tests, fastest / newest / most-fragile / most-interesting first...
  testLibVersion();
  testLibVersions();

  // OTRadioLink
  testFrameDump();
  testCRC7_5B();
  testISRRXQueue1Deep();

  // OTRFM23BLink
  testRFM23B();

  // OTV0p2Base
  testRNG8();


  // Announce successful loop completion and count.
  ++loopCount;
  Serial.println();
  Serial.print(F("%%% All tests completed OK, round "));
  Serial.print(loopCount);
  Serial.println();
  Serial.println();
  Serial.println();
  delay(2000);
  }
