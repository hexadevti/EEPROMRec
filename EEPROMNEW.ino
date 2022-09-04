
#include <string.h>

int CLOCK = 30; 
int IORQ = 28; 
int WR = 26; 
int INT = 24; 
int WAIT = 22; 
int MREQ = 20;
int BUSRQ = 18; 
int RD = 16; 
int BUSAK = 14; 
int RESET = 12; 
int MEM_WR = 10; 



const unsigned int dataPin[] = { 3, 2, 5, 4, 7, 6, 9, 8 };
const unsigned int addressPin[] = { 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 47, 46, 45, 44, 43 };

// Hello World 3e 00 ee 80 d3 ff 18 fa
// Vai e vem 3e 80 d3 01 0f fe 01 28 02 18 f7 d3 01 07 fe 80 28 f0 18 f7

int addressSize = 16;
int16_t currentAddress = 0;
const unsigned int WCT = 7;
int maxAddress = 0;
const boolean logData = false;
int size = 0;
int listCount = 0;
int StringCount = 0;
volatile double clockCount = 0;
char buf[100];
volatile uint16_t addressCount = 0;
uint8_t bufb[80];
String command;
bool mem_read = false;
bool debug = false;
int debugSpeed = 500;

void setup() {
  Serial.begin(250000);
  maxAddress = (int)pow(2, addressSize);
  size = sizeof(addressPin)/sizeof(addressPin[0]);
  initialState();
}

void initialState() {
  
  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    pinMode(addressPin[pin], INPUT);
  }

  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], INPUT);
  }

  pinMode(IORQ, INPUT);
  pinMode(WR, INPUT);
  pinMode(INT, INPUT);
  pinMode(RD, INPUT);
  pinMode(RESET, INPUT);
  pinMode(BUSAK, INPUT);
  pinMode(MREQ, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(WAIT, OUTPUT);
  pinMode(BUSRQ, OUTPUT);
  pinMode(MEM_WR, OUTPUT);
  digitalWrite(MEM_WR, HIGH);
  digitalWrite(BUSRQ, LOW);
  digitalWrite(MREQ, HIGH);
  
  

  Serial.println("OK");
}

void loop() {

  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    if (command.startsWith("c ")) {
      commandClear(command);
    }
    else if (command.equals("l")) {
      commandRead("r");
      currentAddress = currentAddress+256;
    } 
    else if (command.startsWith("r ")) {
      commandRead(command);
    } 
    else if (command.startsWith("w ")) {
      commandWrite(command);
    }
    else if (command.startsWith("d")) {
      preRun();
      if (command.length() > 2) {
        Serial.println(">2");
        Serial.println(command.length());
        String parm = command.substring(2, command.length());
        Serial.println(parm);
        if (parm.length() > 0) {
          Serial.println("parm>0");
          debugSpeed = StrToDec(parm);
          Serial.println(parm);
        }
      }
      
      debug = true;

    }
    else if (command.startsWith("st")) {
      commandStop();
      debug = false;
    }
    else if (command.startsWith("te")) {
      commandTest();
    }
    else 
    {
      commandGo("g " + command);
    }
  }

  if (debug) {
    commandDebug();
    delay(debugSpeed);
  }
}

void commandTest() 
{
  //busRequest(true);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], OUTPUT);
  }
  while (true)
  {
    digitalWrite(MREQ, HIGH);
    writeEEPROM(0x1, 0xff);
    delay(100);
    digitalWrite(MREQ, LOW);
    writeEEPROM(0xffff, 0xff);
    //busRequest(false);
    delay(100);
  }
}

void busRequest(bool active) 
{
  digitalWrite(BUSRQ, active ? LOW : HIGH);
  while (!digitalRead(BUSAK))
  {
    clockCycle(1);
    Serial.println("BUSREQ");
  }
}

void clockCycle(size_t times)
{
  for (size_t i = 0; i < times; i++)
  {
    digitalWrite(CLOCK, HIGH);
    delay(10);
    digitalWrite(CLOCK, LOW);
    delay(10);
  }
  
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
  unsigned int itemCount = 0;
  char buf[60];

  while (strCommand.length() > 0)
  {
    int index = strCommand.indexOf(' ');
    String parm = strCommand.substring(0, index);


    if (parmNo == 1)
    {
      address = StrToDec(parm);
    }
    else if (parmNo > 1)
    {
      //Serial.println();Serial.print("parno: "); Serial.print(parmNo);Serial.print("strCommand: "); Serial.print(strCommand); Serial.println();
      if (itemCount == 0)
      {
        //Serial.println();
        sprintf(buf, "%04x: ", address + (parmNo - 2));
        Serial.print(buf);
      }
      int data = StrToDec(parm);
      writeByte(address + (parmNo - 2), data);
      sprintf(buf, "%02x ", data);
      Serial.print(buf);

      if (itemCount == 7)
      {
        Serial.print("    ");
      }
      if (itemCount == 15)
      {
        Serial.println();
        itemCount = 0;
      }
      else 
      {
        itemCount++;
      }

      if (index == -1)
      {
        Serial.println();
        break;
      }
    }
    strCommand = strCommand.substring(index + 1);
    parmNo = parmNo + 1;
  }
  currentAddress = address;
}

int StrToDec(String parm) {
  int str_len = parm.length() + 1; 
  char char_array[str_len];
  parm.toCharArray(char_array, str_len);
  return (int) strtol(char_array, 0, 16);
}

bool IntRDBegan = false;

void preRun() {
      for (int pin = 0; pin <= 15; pin+=1) {
        pinMode(addressPin[pin], INPUT);
      }
      for (int pin = 0; pin <= 7; pin+=1) {
        pinMode(dataPin[pin], INPUT);
      }
      
      pinMode(MREQ, INPUT);
      digitalWrite(WAIT, HIGH);
      digitalWrite(MEM_WR, HIGH);

      busRequest(false);

}

void commandDebug() {


  digitalWrite(CLOCK, HIGH);
  delay(10);
  digitalWrite(CLOCK, LOW);
  delay(10);
  uint16_t address = readAddress();
  uint8_t data = readData();
  sprintf(buf, "%04X %02X %s %s %s %s %s", address, data, digitalRead(WR) ? "   " : " WR",
                                                                  digitalRead(RD) ? "   " : " RD", 
                                                                  digitalRead(BUSAK) ? "     " : " BUSAK", 
                                                                  digitalRead(IORQ) ? "     " : " IORQ", 
                                                                  digitalRead(RESET) ? "     " : " RESET");
  Serial.println(buf);
  
}

void commandStop() {
      for (int pin = 0; pin <= 15; pin+=1) {
        pinMode(addressPin[pin], INPUT);
      }
      for (int pin = 0; pin <= 7; pin+=1) {
        pinMode(dataPin[pin], INPUT);
      }
      
      pinMode(MREQ, OUTPUT);
      digitalWrite(MREQ, HIGH);
      digitalWrite(WAIT, LOW);
      digitalWrite(MEM_WR, HIGH);

      busRequest(false);

}

void setRead() {
  busRequest(true);
  digitalWrite(MREQ, LOW);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], INPUT);
  }
}

void setWrite() {
  busRequest(true);
  digitalWrite(MREQ, HIGH);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], OUTPUT);
  }
  delay(WCT);
}

void setStandby() {
  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    pinMode(addressPin[pin], INPUT);
  }
  busRequest(false);
  digitalWrite(MREQ, LOW);
}

void setAddress(int address) {
  if (logData) Serial.println("address: " + toBinary(address, 8));

  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    pinMode(addressPin[pin], OUTPUT);
    if (logData) Serial.println("pinMode(" + (String)(addressPin[pin]) + ",OUTPUT)");
  }
  
  String binary;
  for (int i = 0; i < addressSize; i++) {
    digitalWrite(addressPin[i], (address & 1) == 1 ? HIGH : LOW);
    if (logData) Serial.println("digitalWrite(" + (String)(addressPin[i]) + "," + ((address & 1) == 1 ? "HIGH" : "LOW") + ")");
    address = address >> 1;
  }
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

void writeEEPROM(unsigned int address, uint8_t data) {
  setAddress(address);
  if (logData) Serial.println("write: " + (String)(data) + ", " + toBinary(data,8));
  for (int pin = 0; pin <= 7; pin += 1) {
    digitalWrite(dataPin[pin], data & 1);
    if (logData) Serial.println("digitalWrite(" + (String)(dataPin[pin]) + "," + ((data & 1) == 1 ? "HIGH" : "LOW" ) + ")");
    data = data >> 1;
  }
  digitalWrite(MEM_WR, LOW);
  digitalWrite(MEM_WR, HIGH);
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

uint8_t writeData(uint8_t data) {
  uint8_t retData = data;
  
  for (int pin = 0; pin <= 7; pin += 1) {
    digitalWrite(dataPin[pin], data & 1);
    if (logData) Serial.println("digitalWrite(" + (String)(dataPin[pin]) + "," + ((data & 1) == 1 ? "HIGH" : "LOW" ) + ")");
    data = data >> 1;
  }
  
  return retData;
}

