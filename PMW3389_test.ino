// PMW3389 Sensor Test with Motion Pin Support (pin 3)

#include <SPI.h>
#include "pmw3389_firmware.h"  // Include actual firmware definitions

#define Motion_Burst 0x50
#define Delta_X_L    0x03
#define Delta_X_H    0x04
#define Delta_Y_L    0x05
#define Delta_Y_H    0x06
#define Power_Up_Reset 0x3A
#define Config2 0x10
#define SROM_Enable 0x13
#define SROM_Load_Burst 0x62
#define SROM_ID 0x2A
#define Resolution_L 0x0E
#define Resolution_H 0x0F

const int ncs = 4;       // Chip select for SPI
const int motion = 3;    // Motion Trigger (MT) pin from sensor

void adns_com_begin() {
  digitalWrite(ncs, LOW);
}

void adns_com_end() {
  digitalWrite(ncs, HIGH);
}

byte adns_read_reg(byte reg_addr) {
  adns_com_begin();
  SPI.transfer(reg_addr & 0x7F);
  delayMicroseconds(35);
  byte data = SPI.transfer(0);
  adns_com_end();
  delayMicroseconds(19);
  return data;
}

void adns_write_reg(byte reg_addr, byte data) {
  adns_com_begin();
  SPI.transfer(reg_addr | 0x80);
  SPI.transfer(data);
  adns_com_end();
  delayMicroseconds(100);
}

void adns_upload_firmware() {
  adns_write_reg(Config2, 0x00);
  adns_write_reg(SROM_Enable, 0x1D);
  delay(10);
  adns_write_reg(SROM_Enable, 0x18);

  adns_com_begin();
  SPI.transfer(SROM_Load_Burst | 0x80);
  delayMicroseconds(15);
  for (int i = 0; i < firmware_length; i++) {
    SPI.transfer(pgm_read_byte(firmware_data + i));
    delayMicroseconds(15);
  }
  adns_com_end();

  adns_read_reg(SROM_ID);
  adns_write_reg(Config2, 0x00);
}

void setCPI(int cpi) {
  unsigned cpival = cpi / 50;
  adns_write_reg(Resolution_L, (cpival & 0xFF));
  adns_write_reg(Resolution_H, ((cpival >> 8) & 0xFF));
}

void sensor_init() {
  pinMode(ncs, OUTPUT);
  pinMode(motion, INPUT_PULLUP);

  adns_com_end();
  adns_com_begin();
  adns_com_end();

  adns_write_reg(Power_Up_Reset, 0x5A);
  delay(50);

  adns_read_reg(0x02);
  adns_read_reg(Delta_X_L);
  adns_read_reg(Delta_X_H);
  adns_read_reg(Delta_Y_L);
  adns_read_reg(Delta_Y_H);

  adns_upload_firmware();
  setCPI(1000);
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  sensor_init();
  Serial.println("PMW3389 sensor initialized. Move to test...");
}

void loop() {
  // Optionally check motion pin (if used for interrupt-style awareness)
  bool motionDetected = digitalRead(motion) == HIGH;
  if (!motionDetected) return; // Skip reading if no motion detected

  adns_write_reg(Motion_Burst, 0x00);
  delayMicroseconds(35);

  adns_com_begin();
  SPI.transfer(Motion_Burst);
  delayMicroseconds(35);

  byte burst[12];
  for (int i = 0; i < 12; i++) {
    burst[i] = SPI.transfer(0);
    delayMicroseconds(1);
  }
  adns_com_end();

  int dx = (burst[3] << 8) | burst[2];
  int dy = (burst[5] << 8) | burst[4];

  if (dx != 0 || dy != 0) {
    Serial.print("Movement: ");
    if (dy < 0) Serial.print("Forward ");
    if (dy > 0) Serial.print("Backward ");
    if (dx < 0) Serial.print("Left ");
    if (dx > 0) Serial.print("Right ");
    Serial.print(" | dx: "); Serial.print(dx);
    Serial.print(", dy: "); Serial.println(dy);
  }

  delay(50);
}
