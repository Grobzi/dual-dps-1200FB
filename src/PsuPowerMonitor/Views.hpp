#include <MCUFRIEND_kbv.h>
#include <string>

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
  return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// ========== Define color theme =========================
#define	BLACK   rgb(0, 0, 0)
#define	GRAY    rgb(50, 50, 50)
#define	RED     rgb(255, 0, 0)
#define WHITE   rgb(255, 255, 255)

std::string format_precision(float number, uint8_t precision)
{
  const std::string fmt = "%." + std::to_string(precision) + "f";
  int size = std::snprintf(nullptr, 0, fmt.c_str(), number);
  std::string result(size, '\0');
  std::snprintf(result.data(), result.size() + 1, fmt.c_str(), number);
  return result;
}

void draw_banner(MCUFRIEND_kbv & tft, std::string text)
{
  
    int16_t x1, y1;
    uint16_t w, h;
    tft.fillRect(0, 0, tft.width(), 25, GRAY);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((tft.width() - w) / 2, 5);
    tft.println(text.c_str());
}

enum PsuState
{
  normal,
  error
};

class ChangeableText
{
public:
  ChangeableText(MCUFRIEND_kbv & tft, uint16_t x, uint16_t y, uint16_t size, uint16_t foreground, uint16_t background) 
    : _tft(tft), _x(x), _y(y), _size(size), _foreground(foreground), _background(background)
    {
    }

  void set_text(std::string text)
  {
    if(text == _last_text) return;
     // Assumes a monospace font
    _tft.setTextSize(_size);
    _tft.setCursor(_x, _y);
    if(text.length() >= _last_text.length())
    {
      _tft.setTextColor(_foreground, _background);
      _tft.print(text.c_str());
    }
    else
    {
      _tft.setTextColor(_background);
      _tft.print(_last_text.c_str());
      _tft.setCursor(_x, _y);
      _tft.setTextColor(_foreground);
      _tft.print(text.c_str());
    }
    _last_text = text;
  }

  void set_text(std::string text, uint16_t x, uint16_t y)
  {
    if(text == _last_text) return;
    // For text that is "moving" e.g. the Watt text for example it does not clear everything that was there before.
    // Slight flickern as its two writes instead of using the internal write* functions.
    _tft.setTextSize(_size);
    _tft.setCursor(_x, _y);
    _tft.setTextColor(_background);
    _tft.print(_last_text.c_str());
    _x = x;
    _y = y;
    _tft.setCursor(_x, _y);
    _tft.setTextColor(_foreground);
    _tft.print(text.c_str());
    _last_text = text;
  }

private:
  MCUFRIEND_kbv & _tft;
  std::string _last_text;
  uint16_t _x;
  uint16_t _y;
  uint16_t _size;
  uint16_t _foreground;
  uint16_t _background;
};

class PsuView
{
public:
  PsuView(MCUFRIEND_kbv & tft, uint16_t x, uint16_t y) 
    : _tft(tft), _x(x), _y(y),
      field_0(tft, _x + 0, _y + 5, 2, BLACK, WHITE),
      field_1(tft, _x + 80, _y + 5, 2, BLACK, WHITE),
      field_2(tft, _x + 0, _y + 35, 2, BLACK, WHITE),
      field_3(tft, _x + 80, _y + 35, 2, BLACK, WHITE)
  {
  }
  void invalidate() { needs_initial_draw = true; }
  void set_state(PsuState state) 
  {
    if(_state == state) return;
    _state = state;
    invalidate();
    initial_draw();
    if(_state != PsuState::normal) return;
  }
  void initial_draw()
  {
      if(!needs_initial_draw) return;

      if(_state == PsuState::normal)
      {
        _tft.fillRect(_x, _y, 148, 60, WHITE);
        field_0.set_text("");
        field_1.set_text("");
        field_2.set_text("");
        field_3.set_text("");
      }
      if(_state == PsuState::error)
      {
        _tft.fillRect(_x, _y, 148, 60, RED);
        _tft.setTextColor(WHITE);
        _tft.setCursor(_x + 30,  _y + 20);
        _tft.setTextSize(3);
        _tft.print("Error");
      }

      needs_initial_draw = false;
  }

  void update_draw(std::string voltage, std::string current, std::string temperature, std::string power)
  {
    if(_state != PsuState::normal) return;
    field_0.set_text(voltage + "V");
    field_1.set_text(current + "A");
    field_2.set_text(temperature + "C");
    field_3.set_text(power + "W");
  }
private:
  bool needs_initial_draw = true;
  MCUFRIEND_kbv & _tft;
  uint16_t _x;
  uint16_t _y;
  ChangeableText field_0;
  ChangeableText field_1;
  ChangeableText field_2;
  ChangeableText field_3;
  PsuState _state = PsuState::normal;
};

class PsuDetailView
{
public:
  PsuDetailView(MCUFRIEND_kbv & tft, uint16_t x, uint16_t y) 
  : _tft(tft), _x(x + 5), _y(y + 5),
    field_0(tft, _x +  80, _y +   0, 2, BLACK, WHITE),
    field_1(tft, _x +  80, _y +  30, 2, BLACK, WHITE),
    field_2(tft, _x +  80, _y +  60, 2, BLACK, WHITE),
    field_3(tft, _x +  80, _y +  90, 2, BLACK, WHITE),
    field_4(tft, _x +  80, _y + 120, 2, BLACK, WHITE),
    field_5(tft, _x + 250, _y +   0, 2, BLACK, WHITE),
    field_6(tft, _x + 250, _y +  30, 2, BLACK, WHITE),
    field_7(tft, _x + 250, _y +  60, 2, BLACK, WHITE),
    field_8(tft, _x + 250, _y +  90, 2, BLACK, WHITE),
    field_9(tft, _x + 250, _y + 120, 2, BLACK, WHITE),
    field_10(tft, _x + 250, _y + 150, 2, BLACK, WHITE)
  {
  }
  void invalidate() { needs_initial_draw = true; }
  void initial_draw()
  {
      if(!needs_initial_draw) return;

      _tft.fillRect(0, _y - 5, _tft.width(), _tft.height(), WHITE);
      field_0.set_text("");
      field_1.set_text("");
      field_2.set_text("");
      field_3.set_text("");
      field_4.set_text("");
      field_5.set_text("");
      field_6.set_text("");
      field_7.set_text("");
      field_8.set_text("");
      field_9.set_text("");
      field_10.set_text("");

      std::string texts[] = { "V:", "A:", "W:" ,"Temp:" ,"Lufter:", "",
                              "Out:", "Out:", "Out:", "Int:", "Laufz:", "Wh:"
                            };
      
      int i = 0;
      for(int x = 0; x < 2; x++)
      {
        for(int y = 0; y < 6; y++)
        {
          _tft.setTextColor(BLACK);
          _tft.setCursor(_x + x * 180, _y + y * 30);
          _tft.print(texts[i++].c_str());
        }
      }
      needs_initial_draw = false;
  }

  void update_draw(std::string _0, std::string _1, std::string _2, std::string _3, std::string _4, 
                   std::string _5, std::string _6, std::string _7, std::string _8, std::string _9, 
                   std::string _10)
  {
    field_0.set_text(_0);
    field_5.set_text(_1);
    field_1.set_text(_2);
    field_6.set_text(_3);
    field_2.set_text(_4);
    field_7.set_text(_5);
    field_3.set_text(_6);
    field_8.set_text(_7);
    field_4.set_text(_8);
    field_9.set_text(_9);
    field_10.set_text(_10);
  }
private:
  bool needs_initial_draw = true;
  MCUFRIEND_kbv & _tft;
  uint16_t _x;
  uint16_t _y;
  ChangeableText field_0;
  ChangeableText field_1;
  ChangeableText field_2;
  ChangeableText field_3;
  ChangeableText field_4;
  ChangeableText field_5;
  ChangeableText field_6;
  ChangeableText field_7;
  ChangeableText field_8;
  ChangeableText field_9;
  ChangeableText field_10;
};

