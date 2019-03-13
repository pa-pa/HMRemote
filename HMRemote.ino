//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <MultiChannelDevice.h>
#include <Remote.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN  4  // PD4
#define LED_PIN2 5  // PD5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8  // PB0
// Arduino pins for the buttons
// A0,A1,A2,A3 == PIN 14,15,16,17 on Pro Mini
#define BTN01_PIN 14  // PC0
#define BTN02_PIN 3   // PD3
#define BTN03_PIN 15  // PC1
#define BTN04_PIN 7   // PD7
#define BTN05_PIN 9   // PB1
#define BTN06_PIN 6   // PD6
#define BTN07_PIN 16  // PC2
#define BTN08_PIN 17  // PC3
#define BTN09_PIN 18  // PC4
#define BTN10_PIN 19  // PC5

// number of available peers per channel
#define PEERS_PER_CHANNEL 12

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0x29,0x01},       // Device ID
    "papa002901",           // Device Serial
    {0x00,0x29},            // Device Model
    0x01,                   // Firmware Version
    as::DeviceType::Remote, // Device Type
    {0x00,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef DualStatusLed<5,4> LedType;
typedef AskSin<LedType,BatterySensor,RadioType> HalType;
class Hal : public HalType {
  // extra clock to count button press events
  AlarmClock btncounter;
public:
  void init (const HMID& id) {
    HalType::init(id);
    // get new battery value after 50 key press
    battery.init(50,btncounter);
    battery.low(22);
    battery.critical(19);
  }

  void sendPeer () {
    --btncounter;
  }

  bool runready () {
    return HalType::runready() || btncounter.runready();
  }
};

class RemoteList0Data : public List0Data {
  uint8_t BacklightOnTime      : 8;   // 0x0e - 14
  uint8_t BacklightAtKeystroke : 1;   // 0x0d - 13.7
  uint8_t BacklightAtMotion    : 1;   // 0x0d - 13.6

public:
  static uint8_t getOffset(uint8_t reg) {
    switch (reg) {
      case 0x0e: return sizeof(List0Data) + 0;
      case 0x0d: return sizeof(List0Data) + 1;
      default:   break;
    }
    return List0Data::getOffset(reg);
  }

  static uint8_t getRegister(uint8_t offset) {
    switch (offset) {
      case sizeof(List0Data) + 0:  return 0x0e;
      case sizeof(List0Data) + 1:  return 0x0d;
      default: break;
    }
    return List0Data::getRegister(offset);
  }
};

class RemoteList0 : public ChannelList<RemoteList0Data> {
public:
  RemoteList0(uint16_t a) : ChannelList(a) {}

  operator List0& () const {
    return *(List0*)this;
  }

  // from List0
  HMID masterid () { return ((List0*)this)->masterid(); }
  void masterid (const HMID& mid) { ((List0*)this)->masterid(mid); }
  bool aesActive() const { return ((List0*)this)->aesActive(); }
  bool localResetDisable() const { return false; }

  uint8_t backlightOnTime () const { return getByte(sizeof(List0Data) + 0); }
  bool backlightOnTime (uint8_t time) const { return setByte(sizeof(List0Data) + 0,time); }
  bool backlightAtKeystroke () const { return isBitSet(sizeof(List0Data) + 1,1<<7); }
  bool backlightAtKeystroke (bool value) const { return setBit(sizeof(List0Data) + 1,1<<7,value); }
  bool backlightAtMotion () const { return isBitSet(sizeof(List0Data) + 1,1<<6); }
  bool backlightAtMotion (bool value) const { return setBit(sizeof(List0Data) + 1,1<<6,value); }

  void defaults () {
    ((List0*)this)->defaults();
  	backlightOnTime(5);
	  backlightAtKeystroke(true);
	  backlightAtMotion(true);
  }
};

typedef RemoteChannel<Hal,PEERS_PER_CHANNEL,RemoteList0> ChannelType;
typedef MultiChannelDevice<Hal,ChannelType,12,RemoteList0> RemoteType;

Hal hal;
RemoteType sdev(devinfo,0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);

  remoteISR(sdev,1,BTN01_PIN);
  remoteISR(sdev,2,BTN02_PIN);
  remoteISR(sdev,3,BTN03_PIN);
  remoteISR(sdev,4,BTN04_PIN);
  remoteISR(sdev,5,BTN05_PIN);
  remoteISR(sdev,6,BTN06_PIN);
  remoteISR(sdev,7,BTN07_PIN);
  remoteISR(sdev,8,BTN08_PIN);
  remoteISR(sdev,9,BTN09_PIN);
  remoteISR(sdev,10,BTN10_PIN);

  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<> >(hal);
  }
}
