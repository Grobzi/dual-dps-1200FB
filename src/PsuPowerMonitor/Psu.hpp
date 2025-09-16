#include <string>

class Psu
{
public:
  Psu(uint8_t address);
  void update_device_information()
  {
      manufacturer = read_eeprom(0x2C, 5);
      name = read_eeprom(0x32, 26);
      date = read_eeprom(0x1D, 8);
      ct = read_eeprom(0x5B, 14);
      spn = read_eeprom(0x12, 10);
  }

  std::string manufacturer;
  std::string name;
  std::string date;
  std::string ct;
  std::string spn;
  float voltage_in();
  float voltage_out();
  float current_in();
  float current_out();
  float power_in();
  float power_out();
  float temperature_intake();
  float temperature_internal();
  float fan_speed();
  float power_consumed();
  bool connected() { return _connected; }
  int seconds_since_startup();
  static bool is_log_enabled;
private:
  bool _connected = false;
  int _address;
  float read(uint8_t reg, float scale, std::string unit, std::string name);
  float read(uint8_t reg);
  std::string read_eeprom(uint8_t reg, uint8_t length);

  void print(std::string text);
};