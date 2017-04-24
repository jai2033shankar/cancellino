/*  
 *  nRF24L01+ C++ Arduino library - minified version for "cancellino"
 *  Author: Michele Lizzit <michele@lizzit.it> - lizzit.it
 *  v1.1 - 23/4/2017
 *  
 *  Please go to https://lizzit.it/cancellino for more informations about the project
 *  
 *  
 *  Copyright (C) 2017  Michele Lizzit
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <SPI.h>

//Pins
#ifdef BOARD_PROTOTYPE
  #define CSN_PIN 7
  #define CE_PIN 8
#endif
#ifdef BOARD_PCB1
  #define CSN_PIN 6
  #define CE_PIN 5
#endif

#ifdef BOARD_PCB1
  #ifdef BOARD_PROTOTYPE
    #error "Configuration error: you have selected more than one pin scheme"
  #endif
#endif

//NRF ADDRESS
byte NRF_ADDR[3] = { 0x87, 0xC1, 0xB9 }; //a random address

//COMMANDS
#define R_REG 0x00
#define W_REG 0x20
#define RX_PAYLOAD 0x61
#define TX_PAYLOAD 0xA0
#define FLUSH_TX 0xE1
#define FLUSH_RX 0xE2
#define ACTIVATE 0x50

//REG NAMES
#define CONFIG 0x00
#define EN_AA 0x01
#define EN_RXADDR 0x02
#define SETUP_AW 0x03
#define SETUP_RETR 0x04
#define RF_CH 0x05
#define RF_SETUP 0x06
#define STATUS 0x07
#define OBSERVE_TX 0x08
#define CD 0x09
#define RX_ADDR_P0 0x0A
#define RX_ADDR_P1 0x0B
#define RX_ADDR_P2 0x0C
#define RX_ADDR_P3 0x0D
#define RX_ADDR_P4 0x0E
#define RX_ADDR_P5 0x0F
#define TX_ADDR 0x10
#define RX_PW_P0 0x11
#define RX_PW_P1 0x12
#define RX_PW_P2 0x13
#define RX_PW_P3 0x14
#define RX_PW_P4 0x15
#define RX_PW_P5 0x16
#define FIFO_STATUS 0x17
#define DYNPD 0x1C
#define FEATURE 0x1D

void regWrite(byte regNum, byte regVal) {
  digitalWrite(CSN_PIN, LOW);
  SPI.transfer(W_REG | regNum);
  SPI.transfer(regVal);
  digitalWrite(CSN_PIN, HIGH);
}

void sendCommand(byte command) {
  digitalWrite(CSN_PIN, LOW);
  SPI.transfer(command);
  digitalWrite(CSN_PIN, HIGH);
}

void flushBuffers() {
  regWrite(STATUS, 0x70);
  sendCommand(FLUSH_TX);
  sendCommand(FLUSH_RX);
}

void addrWrite(byte regNum, byte len, byte* address) {
  digitalWrite(CSN_PIN, LOW);
  
  SPI.transfer(W_REG | regNum);
  for (byte cnt = 0; cnt < len; cnt++) {
    SPI.transfer(address[cnt]);
  }
  
  digitalWrite(CSN_PIN, HIGH);
}

void setTXMode() {
  digitalWrite(CE_PIN, LOW);
  
  regWrite(CONFIG, 0x0A);
  
  digitalWrite(CE_PIN, HIGH);
  delayMicroseconds(150);
  digitalWrite(CE_PIN, LOW);
}

void powerDownNRF() {
  digitalWrite(CE_PIN, LOW);
  regWrite(CONFIG, 0);
}

void powerUpNRF()
{
  regWrite(CONFIG, 0b00000010);
}

void setPayloadContent(byte num, byte packet[]) {
  digitalWrite(CSN_PIN, LOW);
  
  SPI.transfer(TX_PAYLOAD);
  for (byte cnt = 0; cnt < num; cnt++) {
    SPI.transfer(packet[cnt]);
  }
  
  digitalWrite(CSN_PIN, HIGH);
}

byte regRead(byte regNum) {
  digitalWrite(CSN_PIN, LOW);
  
  SPI.transfer(R_REG | regNum);
  byte regVal = SPI.transfer(0x00);
  
  digitalWrite(CSN_PIN, HIGH);
  
  return regVal;
}

boolean sendStringNRF(unsigned char packet[], short len) {
  setPayloadContent(len, packet);
  setTXMode();

  int cnt = 0;
  byte tmp;
  do {
    tmp = regRead(STATUS);
    #ifndef DEBUG
      cnt++; //to check if the module is not responding
    #endif
  } while (((tmp & 0x20) == 0) and (cnt < 10000));

  regWrite(STATUS, 0x20);
}

void resetRegisters() {
  regWrite(CONFIG, 0x0B); //RX mode
  regWrite(EN_AA, 0x00);
  regWrite(EN_RXADDR, 0x01);
  regWrite(SETUP_AW, 0x01);
  regWrite(SETUP_RETR, 0x00);
  regWrite(RF_CH, 0x01); //RADIO CHANNEL
  regWrite(RF_SETUP, 0x26); //250kbps, 0dBm = 0x26 250kbps, 0dBm (1mW) = 0x06
  regWrite(RX_PW_P0, 0x0A); //Payload size = 10 byte

  addrWrite(RX_ADDR_P0, 3, NRF_ADDR);
  addrWrite(TX_ADDR, 3, NRF_ADDR);

  flushBuffers();
}

void initializeNRF24L01() {  
  pinMode(CSN_PIN, OUTPUT);
  pinMode(CE_PIN, OUTPUT);
  
  SPI.begin();

  SPI.setClockDivider(SPI_CLOCK_DIV64);

  digitalWrite(CSN_PIN, HIGH);
  digitalWrite(CE_PIN, LOW);

  //wait for the nRF24L01+ to start (see datasheet)
  delayMicroseconds(1500);

  resetRegisters();
}

