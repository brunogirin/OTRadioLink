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

Author(s) / Copyright (s): Deniz Erbilgin 2017
*/

/*
 * UNTESTED:
 * - (static) decodeAndHandleRawRXedMessage (single and dual)
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

#include <OTAESGCM.h>
#include <OTRadioLink.h>

namespace OTFHT
{
    uint8_t minuteCount = 0;
    static constexpr uint8_t heatCallPin = 0; // unused in unit tests.

    class NULLSerialStream final : public Stream
      {
      public:
        static bool verbose;
        void begin(unsigned long) { }
        void begin(unsigned long, uint8_t);
        void end();

        virtual size_t write(uint8_t c) override { if(verbose) { fprintf(stderr, "%c\n", (char) c); } return(0); }
        virtual size_t write(const uint8_t *buf, size_t size) override { size_t n = 0; while((size-- > 0) && (0 != write(*buf++))) { ++n; } return(n); }
        virtual int available() override { return(-1); }
        virtual int read() override { return(-1); }
        virtual int peek() override { return(-1); }
        virtual void flush() override { }
      };
    bool NULLSerialStream::verbose = false;

    // Null pollIO
    // FIXME need true version?
    bool pollIO(bool) {return (false);}
    // return fake Key
    bool getFakeKey(uint8_t *key) { memset(key, 0xff, /*OTV0P2BASE::VOP2BASE_EE_LEN_16BYTE_PRIMARY_BUILDING_KEY*/ 16); return (true); }

    // Instantiate objects for templating
    NULLSerialStream ss;
    OTRadioLink::OTNullRadioLink rt;
    OTRadValve::OnOffBoilerDriverLogic<heatCallPin> b0;
    OTRadioLink::OTNullFrameOperationTrue to;
    OTRadioLink::OTNullFrameOperationFalse fo;
}
// Basic sanity/does it compile tests
TEST(FrameHandlerTest, OTFrameDataTest)
{
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[6] = { 5,    0,1,2,3,4 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t decrypted[] = "hello";

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);
    EXPECT_EQ(sizeof(fd.senderNodeID), OTV0P2BASE::OpenTRV_Node_ID_Bytes);
    EXPECT_EQ(sizeof(fd.decryptedBody), OTRadioLink::ENC_BODY_SMALL_FIXED_PTEXT_MAX_SIZE);
    memcpy(fd.senderNodeID, nodeID, sizeof(nodeID));
    memcpy(fd.decryptedBody, decrypted, sizeof(decrypted));
    fd.decryptedBodyLen = sizeof(decrypted);

    EXPECT_EQ(5, fd.msg[-1]);
    EXPECT_EQ(sizeof(decrypted), fd.decryptedBodyLen);

}

TEST(FrameHandlerTest, OTNullFrameOperationTrue)
{
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };
    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);

    EXPECT_TRUE(OTFHT::to.handle(fd));
}
TEST(FrameHandlerTest, OTNullFrameOperationFalse)
{
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };
    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);

    EXPECT_FALSE(OTFHT::fo.handle(fd));
}
// Minimum valid Frame
TEST(FrameHandlerTest, OTSerialFrameOperationSuccess)
{
    OTFHT::NULLSerialStream::verbose = false;
    // Instantiate objects
    OTRadioLink::OTSerialFrameOperation<decltype(OTFHT::ss), OTFHT::ss> so;
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t decrypted[] = { 0, 0x10, '{', 'b', 'c'};

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);
    memcpy(fd.senderNodeID, nodeID, sizeof(nodeID));
    memcpy(fd.decryptedBody, decrypted, sizeof(decrypted));
    fd.decryptedBodyLen = sizeof(decrypted);

    EXPECT_TRUE(so.handle(fd));
}
// Invalid Frame
TEST(FrameHandlerTest, OTSerialFrameOperationFail)
{
    OTFHT::NULLSerialStream::verbose = false;
    // Instantiate objects
    OTRadioLink::OTSerialFrameOperation<decltype(OTFHT::ss), OTFHT::ss> so;
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[6] = { 5,    0,1,3,4,5 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);
    memcpy(fd.senderNodeID, nodeID, sizeof(nodeID));

    // Case (0 != (db[1] & 0x10)
    const uint8_t decrypted0[] = { 0, 0x1, '{', 'b', 'c', 'd'};
    memcpy(fd.decryptedBody, decrypted0, sizeof(decrypted0));
    fd.decryptedBodyLen = sizeof(decrypted0);
    EXPECT_FALSE(so.handle(fd));

    // Case (dbLen > 3)
    const uint8_t decrypted1[] = { 0, 0x10, '{', 'b', 'c', 'd'};
    memcpy(fd.decryptedBody, decrypted1, sizeof(decrypted1));
    fd.decryptedBodyLen = 3;
    EXPECT_FALSE(so.handle(fd));

    // Case ('{' == db[2])
    const uint8_t decrypted2[] = { 0, 0x10, 's', 'b', 'c', 'd'};
    memcpy(fd.decryptedBody, decrypted2, sizeof(decrypted2));
    fd.decryptedBodyLen = sizeof(decrypted2);
    EXPECT_FALSE(so.handle(fd));
}

// Minimum valid Frame
TEST(FrameHandlerTest, OTRelayFrameOperationSuccess)
{
    // Instantiate objects
    OTRadioLink::OTRelayFrameOperation<decltype(OTFHT::rt), OTFHT::rt> ro;
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t decrypted[] = { 0, 0x10, '{', 'b', 'c'};

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);
    memcpy(fd.senderNodeID, nodeID, sizeof(nodeID));
    memcpy(fd.decryptedBody, decrypted, sizeof(decrypted));
    fd.decryptedBodyLen = sizeof(decrypted);

    EXPECT_TRUE(ro.handle(fd));
}

// Invalid Frame
TEST(FrameHandlerTest, OTRelayFrameOperationFail)
{
    // Instantiate objects
    OTRadioLink::OTRelayFrameOperation<decltype(OTFHT::rt), OTFHT::rt> ro;

    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[6] = { 5,    0,1,3,4,5 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};

    // Case nullptr
    const uint8_t decryptedValid[] = { 0, 0x10, '{', 'b', 'c'};
    OTRadioLink::OTFrameData_T fd0(nullptr);
    memcpy(fd0.senderNodeID, nodeID, sizeof(nodeID));
    memcpy(fd0.decryptedBody, decryptedValid, sizeof(decryptedValid));
    fd0.decryptedBodyLen = sizeof(decryptedValid);
    EXPECT_FALSE(ro.handle(fd0));

    // Other cases
    OTRadioLink::OTFrameData_T fd1(&msgBuf[1]);
    memcpy(fd1.senderNodeID, nodeID, sizeof(nodeID));

    // Case (0 != (db[1] & 0x10)
    const uint8_t decrypted0[] = { 0, 0x1, '{', 'b', 'c', 'd'};
    memcpy(fd1.decryptedBody, decrypted0, sizeof(decrypted0));
    fd1.decryptedBodyLen = sizeof(decrypted0);
    EXPECT_FALSE(ro.handle(fd1));

    // Case (dbLen > 3)
    const uint8_t decrypted1[] = { 0, 0x10, '{', 'b', 'c', 'd'};
    memcpy(fd1.decryptedBody, decrypted1, sizeof(decrypted1));
    fd1.decryptedBodyLen = 3;
    EXPECT_FALSE(ro.handle(fd1));

    // Case ('{' == db[2])
    const uint8_t decrypted2[] = { 0, 0x10, 's', 'b', 'c', 'd'};
    memcpy(fd1.decryptedBody, decrypted2, sizeof(decrypted2));
    fd1.decryptedBodyLen = sizeof(decrypted2);
    EXPECT_FALSE(ro.handle(fd1));
}

// Minimum valid Frame
TEST(FrameHandlerTest, OTBoilerFrameOperationSuccess)
{
    // Instantiate objects
    OTRadioLink::OTBoilerFrameOperation<decltype(OTFHT::b0), OTFHT::b0, OTFHT::minuteCount> bo;
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };
    const uint8_t nodeID[OTV0P2BASE::OpenTRV_Node_ID_Bytes] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t decrypted[] = { 0 , 0x10, '{', 'b', 'c'};

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);
    memcpy(fd.senderNodeID, nodeID, sizeof(nodeID));
    memcpy(fd.decryptedBody, decrypted, sizeof(decrypted));
    fd.decryptedBodyLen = sizeof(decrypted);

    EXPECT_TRUE(bo.handle(fd));
}

TEST(FrameHandlerTest, authAndDecodeSecurableFrameBasic)
{
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    0,1,2,3,4 };

    OTRadioLink::OTFrameData_T fd(&msgBuf[1]);

    // (20170614) auth and decode are not implemented and will return true to allow testing other bits.
    const bool test1 = OTRadioLink::authAndDecodeOTSecurableFrame<OTFHT::getFakeKey>(fd);
    EXPECT_TRUE(test1);
}

TEST(FrameHandlerTest, decodeAndHandleOTSecurableFrame)
{
    // message
    // msg buf consists of    { len | Message   }
    const uint8_t msgBuf[] = { 5,    'O',1,2,3,4 };
    const uint8_t * const msgStart = &msgBuf[1];

    //
    const bool test1 = OTRadioLink::decodeAndHandleOTSecureFrame<decltype(OTFHT::to), OTFHT::to,
                                                                    decltype(OTFHT::to), OTFHT::to,
                                                                    OTFHT::getFakeKey>(msgStart);
    EXPECT_FALSE(test1);
}

// Should always return false
TEST(FrameHandlerTest, OTMessageQueueHandlerNull)
{
    OTRadioLink::OTMessageQueueHandlerNull mh;
    EXPECT_FALSE(mh.handle(false, OTFHT::rt));
}

TEST(FrameHandlerTest, OTMessageQueueHandlerBasic)
{
    OTRadioLink::OTMessageQueueHandler<
        OTRadioLink::decodeAndHandleDummyFrame,
        OTRadioLink::decodeAndHandleDummyFrame,
        OTFHT::pollIO, 4800> mh;
    OTRadioLink::OTNullRadioLink rl;
    EXPECT_FALSE(mh.handle(false, rl));
}

// More detailed Tests
