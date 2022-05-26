/*
  EEPROM reader
*/

#include <string.h>

int CE = 19;
int OE = 21;
int WE = 23;

const unsigned int dataPin[] = { 40, 42, 44, 46, 41, 43, 45, 47 };
int addressPin[] = { 16, 14, 12, 10, 8,  6,  4,  2 , 17, 15, 11, 9, 7, 5, 3 };
int addressSize = 15;
const unsigned int WCT = 10;

const boolean logData = false;



void setup() {
  Serial.begin(115200); 

  for (unsigned int pin = 0; pin < addressSize; pin+=1) {
    pinMode(addressPin[pin], OUTPUT);
    if (logData) Serial.println("pinMode(" + (String)(addressPin[pin]) + ",OUTPUT)");
  }

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
  eraseALL();
  delay(1000);
  printContents();
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
  for (unsigned int pin = 0; pin <= 7; pin+=1) {
    pinMode(dataPin[pin], OUTPUT);
  }
  delay(WCT);
}

void setStandby() {
  
  digitalWrite(WE, HIGH);
  digitalWrite(CE, HIGH);
  digitalWrite(OE, HIGH);
  for (unsigned int pin = 0; pin <= addressSize - 1; pin+=1) {
    digitalWrite(addressPin[pin], LOW);
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(WCT);
}

void setAddress(unsigned int address) {
  if (logData) Serial.println("address: " + toBinary(address, 8));
  String binary;
  for (int i = 0; i < addressSize; i++) {
    digitalWrite(addressPin[i], (address & 1) == 1 ? HIGH : LOW);
    if (logData) Serial.println("digitalWrite(" + (String)(addressPin[i]) + "," + ((address & 1) == 1 ? "HIGH" : "LOW") + ")");
    address = address >> 1;
  }
}

uint8_t readEEPROM(unsigned int address) {
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



void printContents() {
  Serial.println("Reading EEPROM Max addresses: " + (String)((int)pow(2, addressSize)));
  Serial.println();
  setRead();

  for (unsigned int base = 0; base < 32768; base += 16) {
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

void eraseALL() {
  Serial.println("Erasing EEPROM");
  setWrite();
  for (unsigned int address = 0; address < 32768; address += 1) {
    writeEEPROM(address, 0xaa);
    if ((address+1) % 128 == 0) {
      Serial.println(address);
    }
  }
  Serial.println(" done");
  setStandby();
}

