#include <Wire.h>
#include <ModbusMaster.h>

HardwareSerial PZEMSerial(2);

// ตั้งค่า pin สำหรับต่อกับ MAX485
#define MAX485_RO  15
#define MAX485_RE  2
#define MAX485_DE  4
#define MAX485_DI  5

// Address ของ PZEM-017 : 0x01-0xF7
static uint8_t pzemSlaveAddr = 0x01;

// ตั้งค่า shunt -->> 0x0000-100A, 0x0001-50A, 0x0002-200A, 0x0003-300A
static uint16_t NewshuntAddr = 0x0001;

ModbusMaster node;

float PZEMVoltage, PZEMCurrent, PZEMPower, PZEMEnergy;

unsigned long startMillisPZEM;                        /* start counting time for LCD Display */
unsigned long currentMillisPZEM;                      /* current counting time for LCD Display */
const unsigned long periodPZEM = 1000;                 // refresh every X seconds (in seconds) in LED Display. Default 1000 = 1 second

unsigned long startMillisReadData;                    /* start counting time for data collection */
unsigned long startMillis1;                           // to count time during initial start up (PZEM Software got some error so need to have initial pending time)

void setupPZEM()
{
  startMillis1 = millis();
  Serial.begin(115200);
  PZEMSerial.begin(9600, SERIAL_8N2, MAX485_RO, MAX485_DI);           // software serial สำหรับติดต่อกับ MAX485

  startMillisPZEM = millis();                           /* Start counting time for run code */
  pinMode(MAX485_RE, OUTPUT);                           /* Define RE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
  pinMode(MAX485_DE, OUTPUT);                           /* Define DE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
  digitalWrite(MAX485_RE, 0);                           /* Arduino create output signal for pin RE as LOW (no output)*/
  digitalWrite(MAX485_DE, 0);                           /* Arduino create output signal for pin DE as LOW (no output)*/

  node.preTransmission(preTransmission);                // Callbacks allow us to configure the RS485 transceiver correctly
  node.postTransmission(postTransmission);
  node.begin(pzemSlaveAddr, PZEMSerial);
  delay(1000);                                          /* after everything done, wait for 1 second */

  // รอครบ 5 วินาที แล้วตั้งค่า shunt และ address
  while (millis() - startMillis1 < 5000) {
    delay(500);
    Serial.print(".");
  }
  
  setShunt(pzemSlaveAddr);                            // ตั้งค่า shunt
  changeAddress(0xF8, pzemSlaveAddr);                 // ตั้งค่า address 0x01 ซื่งเป็นค่า default ของตัว PZEM-017
  // resetEnergy();                                   // รีเซ็ตค่า Energy[Wh] (หน่วยใช้ไฟสะสม)

}

void loopPZEM()
{
  currentMillisPZEM = millis();
  // อ่านค่าจาก PZEM-017
  if (currentMillisPZEM - startMillisPZEM >= periodPZEM)                                            /* for every x seconds, run the codes below*/
  {
    uint8_t result;                                                                                 /* Declare variable "result" as 8 bits */
    result = node.readInputRegisters(0x0000, 6);                                                    /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
    if (result == node.ku8MBSuccess)                                                                /* If there is a response */
    {
      uint32_t tempdouble = 0x00000000;                                                           /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
      PZEMVoltage = node.getResponseBuffer(0x0000) / 100.0;                                       /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
      // 0x0000 to 0x0008 are the register address of the measurement value
      PZEMCurrent = node.getResponseBuffer(0x0001) / 100.0;                                       /* get the 16bit value for the current value, divide it by 100 (as per manual) */

      tempdouble =  (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002);      /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
      PZEMPower = tempdouble / 10.0;                                                              /* Divide the value by 10 to get actual power value (as per manual) */

      tempdouble =  (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004);      /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
      PZEMEnergy = tempdouble;
    }
    else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า -1
    {
      PZEMVoltage = -1;
      PZEMCurrent = -1;
      PZEMPower = -1;
      PZEMEnergy = -1;
    }

    startMillisPZEM = currentMillisPZEM ;                                                       /* Set the starting point again for next counting time */
  }
  
}

void preTransmission()                                                                                    /* transmission program when triggered*/
{
  if (millis() - startMillis1 > 5000)                                                               // Wait for 5 seconds as ESP Serial cause start up code crash
  {
    digitalWrite(MAX485_RE, 1);                                                                     /* put RE Pin to high*/
    digitalWrite(MAX485_DE, 1);                                                                     /* put DE Pin to high*/
    delay(1);                                                                                       // When both RE and DE Pin are high, converter is allow to transmit communication
  }
}

void postTransmission()                                                                                   /* Reception program when triggered*/
{
  if (millis() - startMillis1 > 5000)                                                               // Wait for 5 seconds as ESP Serial cause start up code crash
  {
    delay(3);                                                                                       // When both RE and DE Pin are low, converter is allow to receive communication
    digitalWrite(MAX485_RE, 0);                                                                     /* put RE Pin to low*/
    digitalWrite(MAX485_DE, 0);                                                                     /* put DE Pin to low*/
  }
}

void setShunt(uint8_t slaveAddr)                                                                          //Change the slave address of a node
{

  static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0003;                                                         /* change shunt register address command code */

  uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, slaveAddr);                                                         // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));

  preTransmission();                                                                                /* trigger transmission mode*/

  PZEMSerial.write(slaveAddr);                                                                      /* these whole process code sequence refer to manual*/
  PZEMSerial.write(SlaveParameter);
  PZEMSerial.write(highByte(registerAddress));
  PZEMSerial.write(lowByte(registerAddress));
  PZEMSerial.write(highByte(NewshuntAddr));
  PZEMSerial.write(lowByte(NewshuntAddr));
  PZEMSerial.write(lowByte(u16CRC));
  PZEMSerial.write(highByte(u16CRC));
  delay(10);
  postTransmission();                                                                               /* trigger reception mode*/
  delay(100);
}

void resetEnergy()                                               // reset energy for Meter 1
{
  uint16_t u16CRC = 0xFFFF;                         /* declare CRC check 16 bits*/
  static uint8_t resetCommand = 0x42;               /* reset command code*/
  uint8_t slaveAddr = pzemSlaveAddr;                 // if you set different address, make sure this slaveAddr must change also
  u16CRC = crc16_update(u16CRC, slaveAddr);
  u16CRC = crc16_update(u16CRC, resetCommand);
  preTransmission();                                /* trigger transmission mode*/
  PZEMSerial.write(slaveAddr);                      /* send device address in 8 bit*/
  PZEMSerial.write(resetCommand);                   /* send reset command */
  PZEMSerial.write(lowByte(u16CRC));                /* send CRC check code low byte  (1st part) */
  PZEMSerial.write(highByte(u16CRC));               /* send CRC check code high byte (2nd part) */
  delay(10);
  postTransmission();                               /* trigger reception mode*/
  delay(100);
}

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr)                                            //Change the slave address of a node
{
  static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0002;                                                         /* Modbus RTU device address command code */
  uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, OldslaveAddr);                                                      // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
  preTransmission();                                                                                 /* trigger transmission mode*/
  PZEMSerial.write(OldslaveAddr);                                                                       /* these whole process code sequence refer to manual*/
  PZEMSerial.write(SlaveParameter);
  PZEMSerial.write(highByte(registerAddress));
  PZEMSerial.write(lowByte(registerAddress));
  PZEMSerial.write(highByte(NewslaveAddr));
  PZEMSerial.write(lowByte(NewslaveAddr));
  PZEMSerial.write(lowByte(u16CRC));
  PZEMSerial.write(highByte(u16CRC));
  delay(10);
  postTransmission();                                                                                /* trigger reception mode*/
  delay(100);
}