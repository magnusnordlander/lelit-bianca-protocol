/* 
 * MIT License
 * 
 * Copyright (c) 2016, 2017 Kent A. Vander Velden
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Kent A. Vander Velden
 * kent.vandervelden@gmail.com
 * Arduino Serial Sniffer
 * Originally written May 16, 2016
 */


// The first UART (object Serial) is connected to the USB serial.
// The Mega 2560 has four serial UARTS, Serial, Serial1, ... Serial3
// If there is insufficient number of UART pins, one could try SoftwareSerial
//
// Warning: The UART pins on the Arduinos work with 5V or 3.3V TTL levels only.
// For RS232 voltage levels, use an RS232 transceiver with at least two
// receivers, such as the Maxim MAX208E.


void setup() {
  // All serial connections are 8bit, no parity, and 1 stop bit
  // Assuming each sniffed bytes has three bytes of overhead, such may be the case
  // if forwarding the bytes as ascii, and that the two sniffed serial lines can run
  // duplex, ideally, the serial lines to the sniffing computer would have a 
  // baud rate >= 38400 x 2 x 4
  Serial.begin(115200, SERIAL_8N1); // 115.2K baud serial connection to sniffing computer
  Serial1.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 1
  Serial2.begin(9600, SERIAL_8N1); // 38.4k baud serial sniffed line 2

  Serial.println();
}


void loop() {
  // Three alternative formats for returning data to the sniffing computer are available.
  // loop_ascii() - easiest to read from the serial monitor and easy to parse, especially 
  //                generally at most half-duplex traffic. Includes timestamps
  // loop_ascii_alt() - only one byte written per line. Least effecient method.
  // loop_binary() - most effecient method but will require a program on the sniffing computer to decode
  //
  loop_ascii();
  //loop_ascii_alt();
  //loop_binary();
}


void loop_ascii() {
  // Configure the number of and labels for each serial port
  const int n_ser = 3;
  const char *labels[n_ser] = { "SNIFFER", "RX", "TX" };

  // Create an array of Serial objects to sniff and forward.
  // Data read from Serial objects s[i > 0] will be written to s[0]
  // The Serial objects are type HardwareSerial, derived from Stream
  HardwareSerial *s[n_ser] = {&Serial, &Serial1, &Serial2};
  
  static int last_ser = -1; // last serial port data was received
  byte buf[64];

  // Store start time in microseconds for later use in calculation of timestamp.
  // Milliseconds are unlikely to provide necessary resolution with fast communication.
  // Attaching the serial monitor will cause the Arduino to reset, and micros() returns
  // the number of microseconds since the program started.
  // The micros() value will wrap every 70 hours or so.
  static unsigned long st = 0; 
  if(st == 0) {
    st = micros();
  }
  
  for(int i=1; i<n_ser; i++) {
    // Serial.available() returns the number of bytes stored in the 64-byte receive buffer
    // Incidentally, the write buffer is also 64 bytes
    int n = min(64, s[i]->available());

    if(n > 0) { // if data is available to read
      n = s[i]->readBytes(buf, n);
      
      if(n > 0) { // if data was read
        if(i != last_ser) {
          s[0]->println();

          last_ser = i;        
          unsigned long dt = micros() - st;
          s[0]->print(labels[i]);
          s[0]->print(" ");
          s[0]->print(dt);
          s[0]->print(":");
        }
        
        for(int j=0; j<n; j++) {
          s[0]->print(" ");       
          // Writting as hexidecimal values reduces maximum number width from 
          // three (255) to two (FF)
          s[0]->print(buf[j], HEX);        
        }
        
        // Wait for transmission of forward bytes to finish.
        // Don't see a reason to wait and prevent potential overlap of receive and transmit
        // s[0]->flush();
      }
    }
  }
}


void loop_ascii_alt() {
  // Create an array of Serial objects to sniff and forward.
  // Data read from Serial objects s[i > 0] will be written to s[0]
  // The Serial objects are type HardwareSerial, derived from Stream
  const int n = 3;
  HardwareSerial *s[n] = {&Serial, &Serial1, &Serial2};
  
  bool do_once = true;
  
  for(int i=1; i<n; i++) {
    // Serial.available() returns the number of bytes stored in the 64-byte receive buffer
    if(s[i]->available() > 0) {
      // Serial.read() return the next byte or -1 if no data is available
      int c = s[i]->read();
      
      s[0]->print(i);
      s[0]->print(" ");
      s[0]->println(c);
    }
  }
}


void loop_binary() {
  // Create an array of Serial objects to sniff and forward.
  // Data read from Serial objects s[i > 0] will be written to s[0]
  // The Serial objects are type HardwareSerial, derived from Stream
  const int n = 3;
  HardwareSerial *s[n] = {&Serial, &Serial1, &Serial2};

  // Create a buffer to hold the payload
  const byte header_len = 2 + 1 + 1;
  byte buf[header_len + 64] = { 0xd0, 0x0d, 0, 0 };
  
  for(int i=1; i<n; i++) {
    // Serial.available() returns the number of bytes stored in the 64-byte receive buffer
    int n = min(64, s[i]->available());
    if(n > 0) { // if data is available to read
      n = s[i]->readBytes(&buf[header_len], n);
      if(n > 0) { // if data was read
        buf[2] = i; // update the port number
        buf[3] = n; // update the length
        s[0]->write(buf, header_len + n);

        // The write buffer might not have enough space to receive 
        // the message and could timeout (default Serial.timeout is 1)
      }
    }
  }
}
