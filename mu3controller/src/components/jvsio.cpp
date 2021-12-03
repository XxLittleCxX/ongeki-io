
#include <FastLED.h>

#define LOG(a) void(0)

namespace component
{

  namespace jvsio
  {
    CRGB rightColors[6];
    CRGB leftColors[6];
    CRGB strip[26];
    void start()
    {  
      FastLED.addLeds<WS2812B, 16, GRB>(rightColors, 6);
      FastLED.addLeds<WS2812B, 10, GRB>(leftColors, 6);
      FastLED.show();
      Serial.begin(115200);
    }

    enum
    {
      GET_BOARD_INFO = 0xf0,
      GET_PROTOCOL_VERSION = 0xf3,
      GET_FIRM_SUM = 0xf2,
      GET_BOARD_STATUS = 0xf1,
      SET_TIMEOUT = 0x11,
      RESET = 0x10,
      SET_DISABLE_RESPONSE = 0x14,
      SET_LED_DIRECT = 0x82
    };

    struct Packet
    {
      int address; // dst
      int src_address;
      int length;
      byte message[199];
    };

    struct Reply
    {
      int length;
      byte message[199];
    };

    Packet packet;
    Reply reply[2];
    int curReply = 0;
    int deviceId = -1;

    bool bRecieving = false;
    bool bEscape = false;
    int phase = 0;
    int checksum = 0;
    int cur = 0;

    void ReplyBytes(const byte *bytes, int numBytes)
    {
      Reply *r = &reply[curReply];
      //r->message[r->length++] = 0x01;
      for (int i = 0; i < numBytes; i++)
      {
        r->message[r->length++] = bytes[i];
      }
    }

    void writeEscaped(byte b)
    {
      if (b == 0xE0 || b == 0xD0)
      {
        Serial.write(0xD0);
        b--;
      }
      Serial.write(b);
    }

    void FlushReply()
    {
      Reply *r = &reply[curReply];
      if (r->length > 0)
      {
        int sum = 2 + 1 + 1 + (1 + r->length);
        Serial.write(0xE0);          //sync
        writeEscaped(0x02);          // dstNodeId
        writeEscaped(0x01);          // srcNodeId
        writeEscaped(r->length + 1); //length
        writeEscaped(0x01);          // status
        for (int i = 0; i < r->length; i++)
        {
          sum += r->message[i];
          writeEscaped(r->message[i]);
        }
        writeEscaped(sum & 0xFF); // sum
        curReply = 1 - curReply;
        reply[curReply].length = 0;
      }
    }

    void Resend()
    {
      Reply *r = &reply[curReply];
      Reply *old = &reply[1 - curReply];
      int length = old->length;
      for (int i = 0; i < length; i++)
      {
        r->message[r->length++] = old->message[i];
      }
    }

    byte zeros[64] = {
        0};
    byte tmp[4] = {
        0};

#define Reply() ReplyBytes(NULL, 0)
#define ReplyString(str) ReplyBytes((const byte *)str, sizeof(str)-1)
#define ReplyByte(b)    \
  do                    \
  {                     \
    tmp[0] = b;         \
    ReplyBytes(tmp, 1); \
  } while (0)

    void ProcessPacket(struct Packet *p)
    {
      if (p->address == 0xFF || p->address == deviceId || true) // Yay, it's for me
      {
        int length = p->length;
        byte *message = p->message;
        switch (message[0])
        {
        case GET_BOARD_INFO:
          // fill_solid(rightColors, 6, CRGB::Red);
          // FastLED.show();
          ReplyByte(GET_BOARD_INFO); // command
          ReplyByte(1);              //report
          ReplyString("15093-06");
          ReplyByte(10);
          ReplyString("6710A"); // custom chip
          ReplyByte(255);
          ReplyByte(0xA0); // rev version
          //ReplyByte(10);
          break;
        case GET_PROTOCOL_VERSION:
          // fill_solid(rightColors, 6, CRGB::Orange);
          // FastLED.show();
          ReplyByte(GET_PROTOCOL_VERSION); // command
          ReplyByte(1);                    //report
          ReplyByte(1);                    // applimode
          ReplyByte(1);
          ReplyByte(0);
          break;
        case GET_FIRM_SUM:
          // fill_solid(rightColors, 6, CRGB::Yellow);
          // FastLED.show();
          {
            const auto sum = 0xAA53;
            uint8_t *buf = (uint8_t *)&sum;
            ReplyByte(GET_FIRM_SUM);
            ReplyByte(1);
            ReplyByte(buf[1]); //upper
            ReplyByte(buf[0]); //lower
            break;
          }
        case GET_BOARD_STATUS:
          // fill_solid(rightColors, 6, CRGB::Green);
          // FastLED.show();
          ReplyByte(GET_BOARD_STATUS);
          ReplyByte(1);
          ReplyByte(0);
          ReplyByte(0);
          ReplyByte(0);
          //ReplyByte(0);
          break;
        case SET_TIMEOUT:
          // fill_solid(rightColors, 6, CRGB::Blue);
          // FastLED.show();
          ReplyByte(SET_TIMEOUT);
          ReplyByte(1);
          ReplyByte(p->message[1]); //置空
          ReplyByte(p->message[2]);
          break;
        case RESET:
          // fill_solid(rightColors, 6, CRGB::Purple);
          // FastLED.show();
          ReplyByte(RESET);
          ReplyByte(1);
          ReplyByte(0);
          break;
        case SET_DISABLE_RESPONSE:
          // fill_solid(rightColors, 6, CRGB::GreenYellow);
          // FastLED.show();
          ReplyByte(SET_DISABLE_RESPONSE);
          ReplyByte(1);
          ReplyByte(p->message[1]);
          break;
        case SET_LED_DIRECT:
        {
          uint8_t base = 1 + 59 * 3;
          fill_solid(rightColors, 6, CRGB((uint8_t)(p->message[base]), (uint8_t)(p->message[base + 1]), (uint8_t)(p->message[base + 2])));

          uint8_t leftBase = 1;
          fill_solid(leftColors, 6, CRGB((uint8_t)(p->message[leftBase]), (uint8_t)(p->message[leftBase + 1]), (uint8_t)(p->message[leftBase + 2])));
          /*for (int i = 0; i < 26; i++)
          {
            strip[i].r = p->message[1 + (18 + i) * 3];
            strip[i].g = p->message[1 + (18 + i) * 3 + 1];
            strip[i].b = p->message[1 + (18 + i) * 3 + 2];
          }*/
          FastLED.show();
          /*ReplyByte(SET_LED_DIRECT);
          ReplyByte(1);
          ReplyByte((uint8_t)(p->message[base]));*/
          break;
        }
        default:
          //fill_solid(rightColors, 6, CRGB::Black);
          //FastLED.show();
          // ReplyByte(message[0]);
          // ReplyByte(1);
          //ReplyString("15093-06");
          //ReplyByte(10);

          break;
        }
        /*message += sz;
          length -= sz;
        }
        if (length < 0)
        {
          LOG("Underflowed!");
        }*/
        FlushReply();
      }
      else
      {
        LOG("Not for me");
      }
    }

    void update()
    {
      if (Serial.available())
      {
        uint8_t data = Serial.read();
        //Serial.println(data);
        switch (data)
        {
        case 0xE0: // sync
          bRecieving = true;
          bEscape = false;
          phase = 0;
          cur = 0;
          checksum = 0;
          break;
        case 0xD0: // Escape
          bEscape = true;
          break;
        default:
          if (bEscape)
          {
            data++;
            bEscape = false;
          }
          switch (phase)
          {
          case 0:
            packet.address = data;
            checksum += data;
            phase++;
            break;
          case 1:
            packet.src_address = data;
            checksum += data;
            phase++;
            break;
          case 2:
            packet.length = data;
            checksum += data;
            phase++;
            break;
          case 3:
            if (cur >= packet.length)
            {
              checksum &= 0xFF;
              if (checksum == data)
              {
                ProcessPacket(&packet);
              }
              else
              {
                /*Serial.write("Dropping packet");
                Serial.println(cur);
                Serial.println(packet.length);
                Serial.println(checksum);
                Serial.println(data);*/
              }
              bRecieving = false;
            }
            else
            {
              checksum += data;
              packet.message[cur++] = data;
            }
            break;
          }
          break;
        }
      }
    }
  }
}