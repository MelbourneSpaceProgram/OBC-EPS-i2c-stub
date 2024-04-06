#include <Wire.h>

#define SLAVE_ADDR 0x06
#define PACKET_SIZE 4

// #define ANSWERSIZE 1
uint8_t rxBuf[20];
uint8_t txBuf[4];
uint8_t rxCounter = 0;

uint8_t battery_value;

enum SubSystem : uint8_t{
  ADCS = 0x00,
  EPS = 0x01,
  COMM = 0x02,
  OBC = 0x03,
};

enum CmdType {
  //OBC -> ADCS
  PASSIVE_POINTING_MODE = 0x00,
  ACTIVE_POINTING_MODE = 0x01,

  // ADCS -> OBC
  COMMAND_RESPONSE = 0x10,
  TELEMETRY = 0x11,

  //OBC <-> EPS
  POWER_SUBSYSTEM = 0x20,
  GET_BATTERY = 0x21,

  // EPS -> OBC
  // Telemetry 0x30;

  // EPS -> MPPT
	// 0x40;

  // EPS -> temp tracker
 	// 0x50;
};

enum PowerModes {
  OFF = 0x00,
  ON  = 0x01,


  SYS_FAILURE_OFF= 0xFA,
  SYS_FAILURE_ON = 0xFB,
  SYS_FAILURE_UNKNOWN = 0xFC,
};

struct I2cPacket {
  SubSystem subSystem;
  CmdType cmdType;
  uint8_t val1;
  uint8_t val2;
};

void power_subsystem(uint8_t subsystem, uint8_t mode);
uint8_t get_battery();

/////////////////////
// Main Code
/////////////////////
void setup() {
  //Start I2C as slave
  Wire.begin(SLAVE_ADDR);
  //Attach a function to trigger when something is received
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.begin(9600);

}


void loop() {
  delay(50);
}

/////////////////////
// Functions
/////////////////////
int DeserializeI2cPacket (I2cPacket& p) {
  rxCounter = 0;

  Serial.println("Receive Event");
  while (0 < Wire.available()) 
  {
    rxBuf [rxCounter] = Wire.read();
    rxCounter++;
  }

  if (rxCounter != PACKET_SIZE)
  {
    // incomplete packet structure
    return -1;
  }

  p.subSystem = SubSystem (rxBuf[0]);
  p.cmdType = CmdType (rxBuf[1]);
  p.val1 = rxBuf[2];
  p.val2 = rxBuf[3];

  return 0;
}

void LoadTxBuf (I2cPacket p)
{
  //TODO: Check if buffer is already filled?
  //TODO: Flush buffer after txBuf is sent, then clear buffer in case multiple actions trying to overwrite buffer

  txBuf [0] = (uint8_t) p.subSystem;
  txBuf [1] = (uint8_t) p.cmdType;
  txBuf [2] = p.val1;
  txBuf [3] = p.val2;

}

int SendTxBuf ()
{
  Wire.write(txBuf, PACKET_SIZE);

  return 0;
}

int ProcessRxedPacket (I2cPacket p)
{

  if (p.subSystem != EPS)
  {
    //Packet not for me, drop
    return -1;
  }

  switch (p.cmdType) {
      case POWER_SUBSYSTEM:
      {
        Serial.println("POWER_SUBSYSTEM received");
        power_subsystem(p.val1, p.val2);
        break;
      }

      case GET_BATTERY:
      {
        Serial.println("GET_BATTERY received");
        I2cPacket rxP;
        rxP.subSystem = EPS;
        rxP.cmdType = GET_BATTERY;

        battery_value = get_battery();
        rxP.val1 = battery_value;
        rxP.val2 = 0x01;

        LoadTxBuf (rxP);
        break;
      }
  
      default:
        //unrecognized command
        break;
    }

    return 0;
}

void receiveEvent() {
  int ret;
  I2cPacket p;
  ret = DeserializeI2cPacket (p);
  if (ret < 0)
  {
    //TODO: something wrong, can add error handling here
  }

  ret = ProcessRxedPacket (p);
  if (ret < 0)
  {
    //TODO: something wrong, can add error handling here
  }
}

void requestEvent() {
  Serial.println("Request event");

  SendTxBuf ();
}


void power_subsystem(uint8_t subsystem, uint8_t mode) {
  
  uint8_t retCode;
  switch (CmdType (subsystem))
  {
    case ADCS:
      Serial.println ("Received cmd to power system ADCS.\n");
      retCode = PowerModes::ON;
      break;
    case COMM:
      Serial.println ("Received cmd to power system COMM.\n");
      retCode = PowerModes::ON;
      break;
    case OBC:
      Serial.println ("Received cmd to power system OBC.\n");
      retCode = PowerModes::ON;
      break;
    default:
      Serial.println ("Unrecognized system code, ignore\n");
      retCode = PowerModes::SYS_FAILURE_ON;
      break;
  }
  
  I2cPacket p;
  p.subSystem = EPS;
  p.cmdType = POWER_SUBSYSTEM;
  p.val1 = subsystem;
  p.val2 = retCode;
  LoadTxBuf (p);

  return;
}

uint8_t get_battery() {
  //Stub function for now
  return (uint8_t)random(0,100);
}