#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>

const int sdChipSelect = 48;
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, 46, 47, 49);

uint16_t fileNum = 0;
String fileName;

bool microswitchOn = false;
uint16_t brewboiler = 0;
uint16_t serviceboiler = 0;
uint16_t waterlevel = 0;
bool brewboilerhe = false;
bool serviceboilerhe = false;
bool pumpOn = false;

String ciloError = "";
String coliError = "";

byte coliBuf[18];
byte ciloBuf[5];

unsigned long st;

uint16_t transformHextripet(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    uint32_t triplet = ((uint32_t)byte0 << 16) | ((uint32_t)byte1 << 8) | (uint32_t)byte2;
  
    uint8_t lsb = (triplet & 0x00FE00) >> 9;

    uint8_t hig = (triplet & 0x060000) >> 16;
    uint8_t mid = (triplet & 0x000002) >> 1;

    uint8_t msb = (hig | mid) - 1;

    return (uint16_t)lsb | ((uint16_t)msb << 7);
}

float transformBrewTemp(int brewnumber) {
    float k = -0.1342;
    float m = 119.71;

    return k*((float)brewnumber)+m;
}

float transformServiceTemp(int servicenumber) {
    float k = -0.1402;
    float m = 131.65;

    return k*((float)servicenumber)+m;
}

void setup() {
  u8g2.begin();
  // Open serial communications and wait for port to open:

  Serial1.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 1
  Serial2.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 2

  Serial.print("\nInitializing SD card...");

  if (!SD.begin(sdChipSelect)) {
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    u8g2.drawStr(0,10, "SD Card init failed");
    u8g2.sendBuffer();
    while (true);
  }

  while (1) {
    String fileNumString = String(fileNum);
    fileName = String("log" + fileNumString + ".txt");

     if (fileNum > 1) {
      u8g2.clearBuffer();          // clear the internal memory
      u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
      u8g2.drawStr(0,10, String("Trying file "+fileName).c_str());
      u8g2.sendBuffer();
      delay(50);
    }
    
    if (!SD.exists(fileName)) {
      break;
    }

    fileNum++;
  }

  st = millis();
}

void readData() {
  int n;

  memset(coliBuf, 0, sizeof(coliBuf));
  memset(ciloBuf, 0, sizeof(ciloBuf));
  
  if (Serial1.available() > 0) {
    if (Serial1.available() > 36) {
      // Discard things from the UART buffer until it's less full
      Serial1.readBytesUntil(0x00, coliBuf, 18);
      Serial1.readBytesUntil(0x00, coliBuf, 18);
      memset(coliBuf, 0, sizeof(coliBuf));
    }
    
    // COLI
    n = Serial1.readBytesUntil(0x00, coliBuf, 18);
  
    if(n == 17) {
      microswitchOn = (coliBuf[1] & 0x40) == 0x00;
      brewboiler = transformHextripet(coliBuf[7], coliBuf[8], coliBuf[9]);
      serviceboiler = transformHextripet(coliBuf[10], coliBuf[11], coliBuf[12]);
      waterlevel = transformHextripet(coliBuf[13], coliBuf[14], coliBuf[15]);

      coliError = "";
    } else {
      String lenString = String(n);
      coliError = String("COLI: exp 17B, rcv " + lenString);
    }
  }

  if (Serial2.available() > 0) {
    if (Serial1.available() > 10) {
      // Discard things from the UART buffer until it's less full
      Serial2.readBytesUntil(0x00, ciloBuf, 5);
      Serial2.readBytesUntil(0x00, ciloBuf, 5);
      memset(ciloBuf, 0, sizeof(ciloBuf));
    }
    
    // CILO
    n = Serial2.readBytesUntil(0x00, ciloBuf, 5);
  
    if(n == 4) {
      serviceboilerhe = (ciloBuf[0] & 0x0E) == 0x0A;
      brewboilerhe = (ciloBuf[1] & 0xF0) == 0xE0;
      pumpOn = (ciloBuf[1] & 0x0E) == 0x0E;

      ciloError = "";
    } else {
      String lenString = String(n);
      ciloError = String("CILO: exp 4B, rcv " + lenString);
    }  
  }
}


String fb(bool b) {
  return b ? "On" : "Off";
}

void loop() {
  unsigned long dt = millis() - st;
  readData();

  File dataFile = SD.open(fileName, FILE_WRITE);

  if (dataFile) {
    dataFile.print(String("COLI " + String(dt) + ": ")); 
    for(int j=0; j<sizeof(coliBuf); j++) {
      dataFile.print(" ");       
      dataFile.print(coliBuf[j], HEX);        
    }

    dataFile.println();    

    dataFile.print(String("CILO " + String(dt) + ": ")); 
    for(int j=0; j<sizeof(ciloBuf); j++) {
      dataFile.print(" ");       
      dataFile.print(ciloBuf[j], HEX);        
    }

    dataFile.println(); 
    dataFile.close();
  } else {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0,10, "Error opening file.");
    u8g2.drawStr(0,20, String("File: " + fileName).c_str());
    u8g2.sendBuffer();
    while (true);
  }

  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  
  u8g2.drawStr(0,10, "SW/P");
  u8g2.drawStr(0,20, "Brew");
  u8g2.drawStr(0,30, "Serv");
  u8g2.drawStr(0,40, "B HE");
  u8g2.drawStr(64,10, "Water");
  u8g2.drawStr(64,20, "T");
  u8g2.drawStr(64,30, "T");
  u8g2.drawStr(64,40, "S HE");

  u8g2.drawStr(0,50, ciloError.c_str());
  u8g2.drawStr(0,60, coliError.c_str());

  u8g2.drawStr(40, 10, String(String((int)microswitchOn) + "/" + String((int)pumpOn)).c_str());
  u8g2.drawStr(40, 20, String(brewboiler).c_str());
  u8g2.drawStr(40, 30, String(serviceboiler).c_str());
  u8g2.drawStr(40, 40, fb(brewboilerhe).c_str());

  u8g2.drawStr(104, 10, String(waterlevel).c_str());
  u8g2.drawStr(80, 20, String(String(transformBrewTemp(brewboiler), 1) + "C").c_str());
  u8g2.drawStr(80, 30, String(String(transformServiceTemp(serviceboiler), 1) + "C").c_str());
  u8g2.drawStr(104, 40, fb(serviceboilerhe).c_str());
  
  u8g2.sendBuffer();
}