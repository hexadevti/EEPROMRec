/*
  EEPROM reader
*/

#include <string.h>

int CE = 12;
int OE = 17;
int WE = 44;

const unsigned int dataPin[] = { 7, 5, 3, 2, 4, 6, 8, 10 };
int addressPin[] = { 9, 11, 14, 16, 19, 23, 25, 29, 31, 27, 15, 21, 43, 41, 40 };
int addressSize = 15;
const unsigned int WCT = 10;
int maxAddress = 0;
const boolean logData = false;
byte data[] = {0xAB, 0x5E, 0x6F, 0xB4, 0xEC, 0x88, 0x25, 0x3D, 
               0xC0, 0x12, 0x55, 0x79, 0xBC, 0xAA, 0x11, 0x99};
int size = 0;

void setup() {
  Serial.begin(115200); 
  
  maxAddress = (int)pow(2, addressSize);
  size = sizeof(data)/sizeof(data[0]);
  
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CE, HIGH);
}

// the loop function runs over and over again forever
void loop() {


  //writeByte(0x0000, 0x40);
  //Serial.println(readByte(0x0000), HEX);
  //writeByte(0x0000, 0x30);
  //erase(0x00e0, 0x00ff, 0x00);
  printContents(0x0000, 0x00ff);
  //write(0x0000, data);
  //printContents(0x0000, 0x0001);
  //Serial.println(readByte(0x0000), HEX);

  //delay(500);
  while(1);    
}

void setRead() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CE, LOW);
  digitalWrite(OE, LOW);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], INPUT);
  }
  delay(WCT);
}

void setWrite() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CE, LOW);
  digitalWrite(OE, HIGH);
  for (unsigned int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], OUTPUT);
  }
  delay(WCT);
}

void setStandby() {
  
  digitalWrite(WE, HIGH);
  digitalWrite(CE, HIGH);
  digitalWrite(OE, HIGH);
  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    digitalWrite(addressPin[pin], LOW);
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(WCT);
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
  for (int pin = 0; pin < 8; pin+=1) {
    pinMode(dataPin[pin], INPUT);
  }
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
  digitalWrite(WE, LOW);
  digitalWrite(WE, HIGH);
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
    for (int offset = 0; offset <= 15; offset += 1) {
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
  Serial.println("Erasing EEPROM");
  setWrite();
  for (unsigned int address = startAddress; address <= endAddress; address += 1) {
    writeEEPROM(address, data);
    if ((address+1) % 128 == 0) {
      Serial.println((String)((float)(address+1) / endAddress * 100) + "%");
    }
  }
  Serial.println(" done");
  setStandby();
}

void writeByte(unsigned int startAddress, byte data) {
  setWrite();
  writeEEPROM(startAddress, data);
  setStandby();
}

uint8_t readByte(unsigned int startAddress) {
  setRead();
  return readEEPROM(startAddress);
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

