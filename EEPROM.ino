#include <TimerOne.h>

/*
  EEPROM reader
*/

#include <string.h>
// #include <Wire.h>
#include <SPI.h>


//Pinos Fixos
int CLK_INPUT = 18; // Pino de interrupt de Clock
int CPU_IORQ = 19; // Pino de interrupt de IORQ
int CPU_WR = 20; // Pino Interrupt de Write Request
int FREE_INTERRUPT = 21; // Pino Interrupt de Write Request
int TX3 = 14; // Usado apesar de não referenciado em código
int RX3 = 15; // Usado apesar de não referenciado em código
int TX2 = 16; // Livre para uso
int RX2 = 17; // Livre para uso

int TIMER_ACTIVE = 22; // Ativa o Run - Botão central
int CPU_RESET = 23; // Controla o Reset do processador
int CLK_DISABLE = 24; // Controla clock na placa de clock
int MEM_WE = 25; // Controla o Write Enable da EEPROM
int WRITE_AVAILABLE = 26; //disponibiliza gravação na EEPROM
int CPU_BUSREQ = 27; // Valida o bus para seguir processamento

const unsigned int dataPin[] = { 3, 2, 5, 8, 9, 7, 6, 4 };
const unsigned int addressPin[] = { 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47 };

//int CPU_MREQ = 37;
//int CPU_RD = 38;
//int CPU_M1 = 39;
//int CPU_RFSH = 41;
//int CPU_HALT = 42;
//int CPU_BUSAK = 44;
// int I2C_SDA = 20;
// int I2C_SCL = 21;
// int SPI_SS = 53;
// int SPI_MISO = 50;
// int SPI_MOSI = 51;
// int SPI_SCK = 52;

bool timerActive = false;
int timerButtonPressCount = 0;
bool timerPreviousState = false;



// Hello World 3e 00 ee 80 d3 ff 18 fa
// Vai e vem 3e 80 d3 ff 0f fe 01 28 02 18 f7 d3 ff 07 fe 80 28 f0 18 f7

int addressSize = 16;
int currentAddress = 0;
const unsigned int WCT = 10;
int maxAddress = 0;
const boolean logData = false;
int size = 0;
int listCount = 0;
int StringCount = 0;
volatile double clockCount = 0;
char buf[100];
volatile uint16_t addressCount = 0;
uint8_t bufb[80];
// tv out
//https://www.youtube.com/watch?v=5sFxURFBmtA
// esp 32 VGA
// https://www.youtube.com/watch?v=qJ68fRff5_k
// esp 8266
// https://github.com/smaffer/espvgax2
String command;

//SPISettings settings(16000000, MSBFIRST, SPI_MODE0); 

bool mem_read = false;

void setup() {
  Serial.begin(1000000); 
  Serial3.begin(1000000);
  
  maxAddress = (int)pow(2, addressSize);
  size = sizeof(addressPin)/sizeof(addressPin[0]);
  initialState();
}

void initialState() {
  
  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    pinMode(addressPin[pin], OUTPUT);
  }
  
  pinMode(WRITE_AVAILABLE, OUTPUT);
  // pinMode(CPU_MREQ, INPUT);
  // pinMode(CPU_RD, INPUT);
  // pinMode(CPU_M1, INPUT);
  // pinMode(CPU_RFSH, INPUT);
  // pinMode(CPU_HALT, INPUT);
  pinMode(CPU_IORQ, INPUT);
  pinMode(CPU_WR, INPUT_PULLUP);
  pinMode(CLK_DISABLE, OUTPUT);
  pinMode(CLK_INPUT, INPUT);
  pinMode(MEM_WE, OUTPUT);
  pinMode(CPU_RESET, OUTPUT);
  pinMode(CPU_BUSREQ, OUTPUT);
  //pinMode(CPU_BUSAK, INPUT);
  digitalWrite(MEM_WE, HIGH);
  digitalWrite(WRITE_AVAILABLE, LOW);
  digitalWrite(CLK_DISABLE, HIGH);
  digitalWrite(CPU_RESET, HIGH);
  digitalWrite(CPU_BUSREQ, HIGH);

  detachInterrupt(digitalPinToInterrupt(CLK_INPUT));
  detachInterrupt(digitalPinToInterrupt(CPU_WR));
  
  Serial.println("OK");
}

void loop() {

  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    if (command.startsWith("g ")) {
      commandGo(command);
    } else if (command.startsWith("c ")) {
      commandClear(command);
    }
    else if (command.equals("l")) {
      commandRead("r");
      currentAddress = currentAddress+256;
    } else if (command.equals("p ")) {
      char buf[60];
      sprintf(buf, "%04x", currentAddress);
      Serial.println("");
      Serial.print(buf);
      Serial.println("");
      Serial.println("OK");
    } else if (command.startsWith("r ")) {
      commandRead(command);
    } 
    else if (command.startsWith("w ")) {
      commandWrite(command);
    }
    else if (command.startsWith("ru")) {
      CommandRun();
    }
    else if (command.startsWith("rom")) {
      LoopVideoROM();
    }
    // else if (command.startsWith("spi")) {
    //   preRun();
    //   digitalWrite(SPI_SS, HIGH);
    //   SPI.begin();
    //   attachInterrupt(digitalPinToInterrupt(CPU_WR), clockLoopSpi, FALLING);
    //   reset();
    //   clockCount = 0;
      
    // }
    // else if (command.startsWith("vi")) {
    //   preRun();
    //   attachInterrupt(digitalPinToInterrupt(CPU_IORQ), clockVideo, FALLING);
    //   reset();
    //   clockCount = 0;
    //   addressCount = 0x8000; 
    // }
    else if (command.startsWith("de")) {
      preRun();
      attachInterrupt(digitalPinToInterrupt(CLK_INPUT), clockDebug, RISING);
      clockCount = 0;
    }
    else if (command.startsWith("re")) {
      reset();
    }
    else if (command.startsWith("lo")) {
      commandLoad(command);
    }
    else if (command.startsWith("in")) {
      initialState();
      reset();
    }
    else if (command.startsWith("st")) {
      commandStop();
    }
    else 
    {
      commandGo("g " + command);
    }
  }

  if (timerPreviousState != digitalRead(TIMER_ACTIVE)) {
    timerPreviousState = digitalRead(TIMER_ACTIVE);
    if (!timerPreviousState) {
      timerActive = !timerActive;
      delay(10);
      if (timerActive) {
         CommandRun();
      } else {
        commandStop();
      }
      //Serial.println(timerActive ? "On" : "off");
    }
  }
}

void CommandRun() {
      preRun();
      attachInterrupt(digitalPinToInterrupt(CPU_WR), clockLoop, FALLING);
      reset();
}

void commandStop() {
  reset();
      for (int pin = 0; pin <= 15; pin+=1) {
        pinMode(addressPin[pin], OUTPUT);
      }

      digitalWrite(CLK_DISABLE, HIGH);
      pinMode(MEM_WE, OUTPUT);
      detachInterrupt(digitalPinToInterrupt(CLK_INPUT));
      detachInterrupt(digitalPinToInterrupt(CPU_WR));
      detachInterrupt(digitalPinToInterrupt(CPU_IORQ));
}



void LoopVideoROM() {
     digitalWrite(WRITE_AVAILABLE, LOW); // Desabilita a gravação (Invertido) 34 = C3 = B00000100
     

      for (size_t i = 0; i < 9600; i++)
      {
        uint16_t address = 0x8000 + i;
        setRead();
        uint8_t data = readEEPROM(0x0f80 + i);
        bufb[1] = address & 0xff;
        bufb[0] = (address >> 8);
        bufb[2] = data;
        // sprintf(buf, "%04x%02x",(0x0f80 + i), bufb[2]);
        // Serial.println(buf);
        Serial3.write(bufb, 3);
      }
      digitalWrite(WRITE_AVAILABLE, HIGH); 
}

void commandGo(String strCommand)
{
  unsigned int parmNo = 0;
  while (strCommand.length() > 0)
  {
    int index = strCommand.indexOf(' ');
    String parm = strCommand.substring(0, index);
    if (parmNo == 1)
    {
      currentAddress = StrToDec(parm);
      Serial.println();
      Serial.println("OK");
      break;
    }
    strCommand = strCommand.substring(index + 1);
    parmNo = parmNo + 1;
  }
}

void commandClear(String command) {
  unsigned int parmNo = 0;
  unsigned int startAddress = 0;
  unsigned int endAddress = 0;
  int dat = 0;
   
  while (command.length() > 0)
  {
    int index = command.indexOf(' ');
    String parm = command.substring(0, index);
    if (parmNo == 1)
    {
      startAddress = StrToDec(parm);
    }
    if (parmNo == 2)
    {
      endAddress = StrToDec(parm);
    }
    if (parmNo == 3)
    {
      dat = StrToDec(parm);
      erase(startAddress, endAddress, dat);
      Serial.println();
      Serial.println("OK");
      break;
    }

    command = command.substring(index + 1);
    parmNo = parmNo + 1;
  }
}

void commandRead(String strCommand)
{
  unsigned int parmNo = 0;
  unsigned int startAddress = 0;
  unsigned int endAddress = 0;
  unsigned int itemCount = 1;
  char buf[60];

  while (strCommand.length() > 0)
  {
    int index = strCommand.indexOf(' ');
    String parm = strCommand.substring(0, index);

    if (strCommand.equals("r"))
    {
      setRead();
      startAddress = currentAddress;
      endAddress = currentAddress + 255;
    }

    if (parmNo == 1)
    {
      setRead();
      startAddress = StrToDec(parm);
    }
    else if (parmNo == 2)
    {
      endAddress = StrToDec(parm);
    }
    else
    {
      if (index == -1)
      {
        for (unsigned int i = startAddress; i <= endAddress; i++)
        {
          if (itemCount == 1)
          {
            Serial.println();
            sprintf(buf, "%04x: ", i);
            Serial.print(buf);
          }
          else if (itemCount == 9)
          {
            Serial.print("    ");
          }
          sprintf(buf, "%02x ", readEEPROM(i));
          Serial.print(buf);
          if (itemCount == 16)
          {
            itemCount = 0;
          }
          itemCount++;
        }
        setStandby();
        Serial.println();
        Serial.println("OK");
        break;
      }
    }
    strCommand = strCommand.substring(index + 1);
    parmNo = parmNo + 1;
  }
}

void commandWrite(String strCommand)
{
  unsigned int parmNo = 0;
  unsigned int address = 0;
  unsigned int itemCount = 1;
  char buf[60];

  while (strCommand.length() > 0)
  {
    int index = strCommand.indexOf(' ');
    String parm = strCommand.substring(0, index);

    if (parmNo == 1)
    {
      address = StrToDec(parm);
      Serial.println();
      sprintf(buf, "%04x: ", address);
      Serial.print(buf);
    }
    else if (parmNo > 1)
    {
      int data = StrToDec(parm);
      writeByte(address + (parmNo - 2), data);
      sprintf(buf, "%02x ", data);
      Serial.print(buf);

      if (itemCount == 8)
      {
        Serial.print("   ");
      }
      if (itemCount == 16)
      {
        Serial.println();
        sprintf(buf, "%04x: ", address + (parmNo - 2));
        Serial.print(buf);
        itemCount = 0;
      }
      itemCount++;

      if (index == -1)
      {
        Serial.println();
        break;
      }
    }
    strCommand = strCommand.substring(index + 1);
    parmNo = parmNo + 1;
  }
}

void commandLoad(String command) {
  unsigned int parmNo = 0;
  unsigned int startAddress = 0;
  unsigned int endAddress = 0;
  int dat = 0;
   
  while (command.length() > 0)
  {
    int index = command.indexOf(' ');
    String parm = command.substring(0, index);
    if (parmNo == 1)
    {
      startAddress = StrToDec(parm);
    }
    if (parmNo == 2)
    {
      endAddress = StrToDec(parm);
    }
    if (parmNo == 3)
    {
      dat = StrToDec(parm);
      streamData(startAddress, endAddress);
      break;
    }

    command = command.substring(index + 1);
    parmNo = parmNo + 1;
  }
}

int StrToDec(String parm) {
  int str_len = parm.length() + 1; 
  char char_array[str_len];
  parm.toCharArray(char_array, str_len);
  return (int) strtol(char_array, 0, 16);
}




void clockLoop() {
    digitalWrite(CPU_BUSREQ, LOW);
    uint16_t address = readAddress();
    if (address >= 0x8000 && address <= 0xa580)
    {
      uint8_t data = readData();
      bufb[1] = address & 0xff;
      bufb[0] = (address >> 8);
      bufb[2] = data;
      //Serial.write(bufb, 3);
      //sprintf(buf, "%04x%02x", address, data);
      //sprintf(buf, "%02x%02x%02x",bufb[0], bufb[1], bufb[2]);
    	//Serial.println(buf);
      Serial3.write(bufb, 3);
    }
    digitalWrite(CPU_BUSREQ, HIGH);
}

// int countScreen = 0;
// int countAddress = 0;

// void clockLoopSpi() {
//     digitalWrite(SPI_SS, LOW);
//     uint16_t address = readAddress();
//     // uint16_t address = 0x8000 + countAddress; 
//     // if (address == 0x8000) countScreen = countScreen + 1;
//     uint8_t data = readData();
//     bufb[1] = address & 0xff;
//     bufb[0] = (address >> 8);
//     bufb[2] = data;
//     SPI.beginTransaction(settings);
//     SPI.transfer(bufb, 3);
//     SPI.endTransaction();  
//     digitalWrite(SPI_SS, HIGH);
//     // countAddress++;
//     // if (countAddress > 0x2580) 
//     //   countAddress = 0; 
// }



// void clockVideo() {
//    uint8_t data = readData();
//     //uint16_t address = readAddressBits(8);
    
// //    if (address == 0x2 && data != dataant) {
//     //Serial.print(clockCount, 0);
//     sprintf(buf, "%04x%02x",addressCount, data);
//     Serial.println(buf);
//     addressCount++;
// //    dataant = data;
// //    }
// }

void preRun() {
      for (int pin = 0; pin <= 15; pin+=1) {
        pinMode(addressPin[pin], INPUT);
      }
      for (int pin = 0; pin <= 7; pin+=1) {
        pinMode(dataPin[pin], INPUT);
      }
      pinMode(MEM_WE, INPUT);
      digitalWrite(WRITE_AVAILABLE, LOW);
      digitalWrite(CLK_DISABLE, LOW);

      detachInterrupt(digitalPinToInterrupt(CLK_INPUT));
      detachInterrupt(digitalPinToInterrupt(CPU_WR));
      detachInterrupt(digitalPinToInterrupt(CPU_IORQ));

}

void reset() {
  digitalWrite(CPU_RESET, LOW);
  delay(500);
  digitalWrite(CPU_RESET, HIGH);  
  mem_read = false;
}

void clockDebug() {
  uint16_t address = readAddress();
  uint8_t data = readData();

  if (logData) {
    Serial.print((String)toBinary(address, 16));
    Serial.print("   ");
    Serial.print((String)toBinary(data, 8));
    Serial.print("   "); 
  }
  //Serial.print(clockCount, 0);
  sprintf(buf, " %s %04X %02X %s %s", digitalRead(MEM_WE) ? "R" : "W", address, data, digitalRead(MEM_WE) ? "   " : " WR", digitalRead(CPU_IORQ) ? "     " : " IORQ");
  //sprintf(buf, " %s %04X %02X %s %s %s %s %s %s %s", digitalRead(MEM_WE) ? "R" : "W", address, data, digitalRead(MEM_WE) ? "   " : " WR", digitalRead(CPU_MREQ) ? "     " : " MREQ", digitalRead(CPU_RD) ? "   " : " RD", digitalRead(CPU_M1) ? "   " : " M1", digitalRead(CPU_RFSH) ? "     " : " RFSH",  digitalRead(CPU_IORQ) ? "     " : " IORQ", digitalRead(CPU_HALT) ? "     " : " HALT");
  //sprintf(buf, "%s %04x %02x ", digitalRead(MEM_WE) ? "R" : "W", address, data);
  Serial.println(buf);
  if (!digitalRead(MEM_WE) && address >= 0x8000 && address < 0xa580) {
      // sprintf(buf, "%04x%02x", address, data);
      // Serial.println(buf);
      // uint8_t data = readData();
      bufb[1] = address & 0xff;
      bufb[0] = (address >> 8);
      bufb[2] = data;
      Serial3.write(bufb, 3);
    }
  clockCount++;
}

void clockCycleCount() {
  clockCount++;
}


void setRead() {
  //digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(MEM_WE, HIGH);
  //digitalWrite(CE, LOW);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], INPUT);
  }
}

void setReadFast() {
  //digitalWrite(MEM_WE, HIGH); // L1
  PORTL = PORTL | B00000010;
  //digitalWrite(MEM_OE, LOW); // L0
  PORTL = PORTL & (~B00000001);
  // for (unsigned int pin = 0; pin < 8; pin+=1) {
  //   pinMode(dataPin[pin], INPUT);
  // }

  DDRE = DDRE | B00010000; // 2    
  DDRE = DDRE | B00001000; // 5
  DDRE = DDRE | B00100000; // 3
  DDRH = DDRH | B00100000; // 8
  DDRH = DDRH | B01000000; // 9
  DDRH = DDRH | B00010000; // 7
  DDRH = DDRH | B00001000; // 6
  DDRG = DDRG | B00100000; // 4
}




void setWrite() {
  digitalWrite(MEM_WE, HIGH);
  digitalWrite(WRITE_AVAILABLE, HIGH);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], OUTPUT);
  }
  delay(WCT);
}

void setStandby() {
  digitalWrite(WRITE_AVAILABLE, LOW);
  // digitalWrite(WE, HIGH);
  // //digitalWrite(CE, HIGH);
  // digitalWrite(OE, HIGH);
  // for (unsigned int pin = 0; pin < addressSize; pin+=1) {
  //   digitalWrite(addressPin[pin], LOW);
  // }
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(WCT);
}

void setAddress(int address) {
  if (logData) Serial.println("address: " + toBinary(address, 8));

  // for (unsigned int pin = 0; pin < addressSize; pin+=1) {
  //   pinMode(addressPin[pin], OUTPUT);
  //   if (logData) Serial.println("pinMode(" + (String)(addressPin[pin]) + ",OUTPUT)");
  // }
  
  String binary;
  for (int i = 0; i < addressSize; i++) {
    digitalWrite(addressPin[i], (address & 1) == 1 ? HIGH : LOW);
    if (logData) Serial.println("digitalWrite(" + (String)(addressPin[i]) + "," + ((address & 1) == 1 ? "HIGH" : "LOW") + ")");
    address = address >> 1;
  }
}

void setAddressFast(int address) {
  PORTC = (address & 1) == 1 ? PORTC | B00001000 : PORTC & (~B00001000); // 31 C3
  address = address >> 1;
  PORTC = (address & 1) == 1 ? PORTC | B10000000 : PORTC & (~B10000000); // 30 C7
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B10000000 : PORTC & (~B10000000); // 29 A7
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B01000000 : PORTC & (~B01000000); // 28 A6 
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00100000 : PORTC & (~B00100000); // 27 A5
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00010000 : PORTC & (~B00010000); // 26 A4
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00001000 : PORTC & (~B00001000); // 25 A3
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00000100 : PORTA & (~B00000100); // 24 A2
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00000010 : PORTA & (~B00000010); // 23 A1
  address = address >> 1;
  PORTA = (address & 1) == 1 ? PORTA | B00000001 : PORTA & (~B00000001); // 22 A0
  address = address >> 1;
  PORTH = (address & 1) == 1 ? PORTH | B00000001 : PORTH & (~B00000001); // 17 H0
  address = address >> 1;
  PORTH = (address & 1) == 1 ? PORTH | B00000010 : PORTH & (~B00000010); // 16 H1
  address = address >> 1;
  PORTB = (address & 1) == 1 ? PORTB | B10000000 : PORTB & (~B10000000); // 13 B7
  address = address >> 1;
  PORTB = (address & 1) == 1 ? PORTB | B01000000 : PORTB & (~B01000000); // 12 B6
  address = address >> 1;
  PORTB = (address & 1) == 1 ? PORTB | B00100000 : PORTB & (~B00100000); // 11 B5
  address = address >> 1;
  PORTB = (address & 1) == 1 ? PORTB | B00010000 : PORTB & (~B00010000); // 10 B4
}


uint8_t readEEPROM(int address) {
  setAddress(address);
  uint8_t data = 0;
  for (int pin = 7; pin >= 0; pin -= 1) {
    data = (data << 1) + digitalRead(dataPin[pin]);
  }
  if (logData) Serial.println("read: " + (String)(data) + ", " + toBinary(data, 8));
  return data;
}

uint8_t readEEPROMFast(int address) {
  setAddressFast(address);
  uint8_t data = 0;
  data = (data << 1) + ((PINE & B00010000) == 0 ? 0 : 1); // 2 E4
  data = (data << 1) + ((PINE & B00001000) == 0 ? 0 : 1); // 5 E3
  data = (data << 1) + ((PINE & B00100000) == 0 ? 0 : 1); // 3 E5
  data = (data << 1) + ((PINH & B00100000) == 0 ? 0 : 1); // 8 H5
  data = (data << 1) + ((PINH & B01000000) == 0 ? 0 : 1); // 9 H6
  data = (data << 1) + ((PINH & B00010000) == 0 ? 0 : 1); // 7 H4
  data = (data << 1) + ((PINH & B00001000) == 0 ? 0 : 1); // 6 H3
  data = (data << 1) + ((PING & B00100000) == 0 ? 0 : 1); // 4 G5
  return data;
}

void writeEEPROM(unsigned int address, uint8_t data) {
  setAddress(address);
  if (logData) Serial.println("write: " + (String)(data) + ", " + toBinary(data,8));
  for (int pin = 0; pin <= 7; pin += 1) {
    digitalWrite(dataPin[pin], data & 1);
    if (logData) Serial.println("digitalWrite(" + (String)(dataPin[pin]) + "," + ((data & 1) == 1 ? "HIGH" : "LOW" ) + ")");
    data = data >> 1;
  }
  digitalWrite(MEM_WE, LOW);
  digitalWrite(MEM_WE, HIGH);
  delay(WCT);
}

String toBinary(int n, int len)
{
    String binary;

    for (unsigned i = (1 << len - 1); i > 0; i = i / 2) {
        binary += (n & i) ? "1" : "0";
    }
 
    return binary;
}

void printContents(unsigned int startAddress, unsigned int endAddress ) {
  Serial.println("Reading EEPROM Max addresses: " + (String)((int)pow(2, addressSize)));
  Serial.println();
  setRead();

  for (unsigned int base = startAddress; base < endAddress; base += 16) {
    uint16_t data[16];
    for (int offset = 0; offset < addressSize; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }
    char buf[60];
    sprintf(buf, "%04x: %02x %02x %02x %02x %02x %02x %02x %02x     %02x %02x %02x %02x %02x %02x %02x %02x",
    base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], 
    data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    Serial.println(buf);
  }

  setStandby();
}

void erase(unsigned int startAddress, unsigned int endAddress, uint8_t data) {
  setWrite();
  for (unsigned int address = startAddress; address <= endAddress; address += 1) {
    writeEEPROM(address, data);
    if ((address+1) % 128 == 0) {
      Serial.println((String)((float)(address+1) / endAddress * 100) + "%");
    }
  }
  setStandby();
}

void streamData(unsigned int startAddress, unsigned int endAddress) {
  setRead();
  for (unsigned int address = startAddress; address <= endAddress; address += 1) {
    uint8_t data = readEEPROM(address);
    //Serial.write(address);

    // char buf2[6];
    // sprintf(buf2, "%04x%02x",address, data);
    //Serial.println(buf2);
  }
  setStandby();
}

void writeByte(unsigned int startAddress, byte data) {
  setWrite();
  writeEEPROM(startAddress, data);
  setStandby();
}

uint8_t readByte(unsigned int address) {
  setRead();
  return readEEPROM(address);
  setStandby();
}

void write(unsigned int startAddress, byte data[]) {
  Serial.println("Writing EEPROM");
  setWrite();

  for (unsigned int a = 0; a < size; a = a + 1)
  {
    if (a % 128 == 0) {
      Serial.println((String)((float)(startAddress + a) / (startAddress + size) * 100) + "%");
    }
    writeEEPROM(startAddress + a, data[a]);
  } 
  Serial.println(" done");
  setStandby();
}

uint16_t readAddress() {
  uint16_t address = 0;
  for (int pin = 15; pin >= 0; pin -= 1) {
    address = (address << 1) + digitalRead(addressPin[pin]);
  }
  if (logData) Serial.println("readAddress: " + (String)(address) + ", " + toBinary(address, 16));
  return address;
}

uint16_t readAddressBits(uint8_t bits) {
  uint16_t address = 0;
  for (int pin = bits; pin > 0; pin -= 1) {
    address = (address << 1) + digitalRead(addressPin[pin]);
  }
  return address;
}

uint8_t readData() {
  uint8_t data = 0;
  // for (unsigned int pin = 0; pin <= 7; pin+=1) {
  //   pinMode(dataPin[pin], INPUT);
  //   if (logData) Serial.println("pinMode(" + (String)(dataPin[pin]) + ",INPUT)");
  // }
  for (int pin = 7; pin >= 0; pin -= 1) {
    data = (data << 1) + digitalRead(dataPin[pin]);
  }
  //if (logData) Serial.println("readData: " + (String)(data) + ", " + toBinary(data, 8));
  return data;
}

