void setup() {
  Serial.begin(115200, SERIAL_8N1); // 115.2K baud serial connection to sniffing computer
  Serial1.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 1
  Serial2.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 2

  Serial.println();
}

uint16_t transformHextripet(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    uint32_t triplet = ((uint32_t)byte0 << 16) | ((uint32_t)byte1 << 8) | (uint32_t)byte2;
  
    uint8_t lsb = (triplet & 0x00FE00) >> 9;

    uint8_t hig = (triplet & 0x060000) >> 16;
    uint8_t mid = (triplet & 0x000002) >> 1;

    uint8_t msb = (hig | mid) - 1;

    return (uint16_t)lsb | ((uint16_t)msb << 7);
}

float transformBrewTemp(int brewnumber) {
    // 754 = 24 deg C
    // 173 = 93 deg C

    float k = ((93. - 24.) / (754. - 173.))*-1.;
    float m = ((k * 754.) - 24.)*-1.;

    return k*((float)brewnumber)+m;
}

float transformServiceTemp(int servicenumber) {
    // 735 = 24 deg C
    // 63 = 125 deg C

    float k = ((125. - 24.) / (735. - 63.))*-1.;
    float m = ((k * 735.) - 24.)*-1;

    return k*((float)servicenumber)+m;
}

void loop() {
  const int n_ser = 3;
  const char *labels[n_ser] = {"UART", "COLI", "CILO" };

  HardwareSerial *s[n_ser] = {&Serial, &Serial1, &Serial2};
  
  byte coliBuf[18];
  byte ciloBuf[5];

  int n;
  
  if (s[1]->available() > 0) {
    // COLI
    n = s[1]->readBytesUntil(0x00, coliBuf, 18);
  
    if(n == 17 && coliBuf[0] == 0x3F) {
              bool microswitchOn = (coliBuf[1] & 0x40) == 0x00;
              uint16_t brewboiler = transformHextripet(coliBuf[7], coliBuf[8], coliBuf[9]);
              uint16_t serviceboiler = transformHextripet(coliBuf[10], coliBuf[11], coliBuf[12]);
              uint16_t waterlevel = transformHextripet(coliBuf[13], coliBuf[14], coliBuf[15]);
  
              s[0]->print("Microswitch: ");
              s[0]->print(microswitchOn ? "On" : "Off");
              s[0]->print(" Brew boiler num: ");
              s[0]->print(brewboiler);
              s[0]->print(" Brew boiler temp: ");
              s[0]->print(transformBrewTemp(brewboiler));
              s[0]->print(" Service boiler num: ");            
              s[0]->print(serviceboiler);
              s[0]->print(" Service boiler temp: ");            
              s[0]->print(transformServiceTemp(serviceboiler));
              s[0]->print(" Water level num: ");            
              s[0]->print(waterlevel);
              s[0]->print(" ");
    } else {
              s[0]->print("Unexpected length. Reading COLI, expecting 18 bytes, received ");
              s[0]->println(n);              
    }
  }

  if (s[2]->available() > 0) {
    // CILO
    n = s[2]->readBytesUntil(0x00, ciloBuf, 5);
  
    if(n == 4) {
              bool brewboilerhe = (ciloBuf[0] & 0x0E) == 0x0A;
              bool serviceboilerhe = (ciloBuf[1] & 0xF0) == 0xE0;
              bool pumpOn = (ciloBuf[1] & 0x0E) == 0x0E;
  
              s[0]->print("Brew boiler HE: ");
              s[0]->print(brewboilerhe ? "On" : "Off");
              s[0]->print(" Service boiler HE: ");
              s[0]->print(serviceboilerhe ? "On" : "Off");
              s[0]->print(" Pump: ");            
              s[0]->println(pumpOn ? "On" : "Off");
    } else {
              s[0]->print("Unexpected length. Reading CILO, expecting 5 bytes, received ");
              s[0]->println(n);
    }  
  }

}