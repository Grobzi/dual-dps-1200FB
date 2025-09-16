#include "Psu.hpp"

#include <Wire.h>

bool Psu::is_log_enabled = false;

Psu::Psu(uint8_t address) : _address(address)
{
  //The address of the eeprom will be 8 lower
}

float Psu::voltage_in()
{
  return read(0x04, 32.0f, "V", "Input voltage");
}

float Psu::voltage_out()
{
  return read(0x07, 256.0f, "V", "Output voltage");
}

float Psu::current_in()
{
  return read(0x05, 128.0f, "A", "Input current");
}

float Psu::current_out()
{
  return read(0x08, 128.0f, "A", "Output current");
}

float Psu::power_in()
{
  return read(0x06, 2.0f, "W", "Input power");
}

float Psu::power_out()
{
  return read(0x09, 2.0f, "W", "Input power");
}

float Psu::temperature_intake()
{
  return read(0x0d, 64.0f, "°C", "Temp (intake)");
}

float Psu::temperature_internal()
{
  return read(0x0e, 64.0f, "°C", "Temp (intern)");
}

float Psu::fan_speed()
{
  return read(0x0f, 1.0f, "rpm", "Fan speed");
}

float Psu::power_consumed()
{
  // It is not the SI unit but Wh is more commonly used.
  read(0x16, -4.0f * 3600, "Wh", "Power consumed");
}

int Psu::seconds_since_startup()
{
  return (int)read(0x18, 2.0f, "s", "Seconds since startup");
}

float Psu::read(uint8_t reg, float scale, std::string unit, std::string name)
{
  float value = read(reg);
  if(scale < 0)
  {
    scale = -scale;
    float value_1 = read(reg + 1);
    value = (int32_t)value_1 << 16 | (int32_t)value;  
  }
  if(value < 0) return value;
  value /= scale;
  print(std::string(name + "\t" + std::to_string(value) + unit));
  return value;
}

float Psu::read(uint8_t reg)
{
  if(!_connected)
    return -1;

  //calculate essential checksum that is part of data request instruction
  uint8_t checksum = (reg << 1) + (_address << 1);
  uint8_t reg_checksum = ((0xff-checksum)+1)&0xff;
  Wire.beginTransmission(_address); 
  Wire.write(reg << 1);
  Wire.write(reg_checksum);
  uint8_t ret = Wire.endTransmission();

  if(ret)
  {
    print("Failed to read register: " + std::to_string(reg) + ". Error:" + std::to_string(ret));
    _connected = false;
    return -1;
  }
  
  uint8_t n = Wire.requestFrom(_address, (uint8_t)2);
  int32_t lsb = Wire.read();
  int32_t msb = Wire.read();
  return (float)(msb << 8 | lsb);
}

void Psu::print(std::string text)
{
  if(!Psu::is_log_enabled) return;
  if (_address < 16)
    Serial.print("0");
  Serial.print("PSU ");
  Serial.print(_address, HEX);
  Serial.println(std::string(": " + text).c_str());
}

std::string Psu::read_eeprom(uint8_t reg, uint8_t length)
{
  uint8_t eeprom_address = _address - 8;
  Wire.beginTransmission(eeprom_address);
  Wire.write(reg);
  Wire.endTransmission();
  uint8_t ret = Wire.endTransmission();
  if(ret)
  {
    print("Failed to read eeprom: " + std::to_string(reg) + ". Error:" + std::to_string(ret));
    return "";
  }
  _connected = true;
  int size = Wire.requestFrom(eeprom_address, length);
  std::string result;
  result.reserve(size);
  while (Wire.available() && (result.size() <= size))
  {
    result += Wire.read();
  }
  print(result);
  return result;
}
