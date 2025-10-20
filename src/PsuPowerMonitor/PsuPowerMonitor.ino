#include "Psu.hpp"
#include "TouchScreen_kbv.h"
#include "Views.hpp"

#include <MCUFRIEND_kbv.h>
#include <Wire.h>
#include <vector>

#define TouchScreen TouchScreen_kbv
#define TSPoint TSPoint_kbv
#define XP 8
#define YP A3
#define XM A2
#define YM 9
#define ENABLE_PIN D1

Psu psu_0 = Psu(0x5F);
Psu psu_1 = Psu(0x5C);
MCUFRIEND_kbv tft;
TouchScreen ts(8, YP, XM, YM, 300);
TSPoint tp;
PsuView view_0 = PsuView(tft, 5, 30);
PsuView view_1 = PsuView(tft, 168, 30);
PsuDetailView detail_view = PsuDetailView(tft, 0, 25);
ChangeableText voltage_text = ChangeableText(tft, 5, 112, 4, BLACK, WHITE);
ChangeableText current_text = ChangeableText(tft, 168, 112, 4, BLACK, WHITE);
ChangeableText power_text = ChangeableText(tft, 180, 175, 7, WHITE, GRAY);
ChangeableText power_combined_text = ChangeableText(tft, 180, 175, 1, WHITE, GRAY);
std::vector<uint32_t> consumption_per_hour = {0};

void setup()
{
    Wire.setTimeout(10);
    Wire.begin();
    Serial.begin(9600);
    pinMode(ENABLE_PIN, PinMode::OUTPUT);
    // while (!Serial);
    tft.begin();
    tft.setRotation(1);
    tft.cp437(true);
}

// State machine variables
enum State
{
    Init,
    Overview,
    Detail,
    Statistics,
    Settings
};
State state = State::Init;
State last_state = State::Init;
int8_t selected_psu;

void display_loop()
{
    static uint32_t last_draw_time = 0;
    uint32_t time = millis();
    if (last_state == state)
    {
        if (time < last_draw_time)
            last_draw_time = 0;
        if (time - last_draw_time < 500)
            return;
    }
    last_draw_time = time;

    if (state == State::Init)
    {
        last_state = state;
        tft.fillScreen(RED);
        tft.setCursor(10, 10);
        tft.setTextColor(WHITE);
        tft.setTextSize(1);
        tft.print("Connect to PSU 1... ");
        delay(100);
        psu_0.connect();
        tft.print(psu_0.connected() ? psu_0.spn.c_str() : "FAILED");
        tft.setCursor(10, 20);
        tft.print("Connect to PSU 2... ");
        delay(100);
        psu_1.connect();
        tft.print(psu_1.connected() ? psu_1.spn.c_str() : "FAILED");
        delay(100);
        if (!psu_0.connected() || !psu_1.connected())
        {
            delay(500);
        }

        state = State::Overview;
    }

    if (state == State::Overview)
    {
        if (last_state != state)
        {
            last_state = state;
            draw_banner(tft, "UBERSICHT");
            tft.fillRect(0, 25, tft.width(), tft.height(), WHITE);
            tft.fillRect(tft.width() / 2 - 1, 25, 3, 70, GRAY);
            tft.fillRect(0, 95, tft.width(), 3, GRAY);
            tft.fillRect(0, 155, tft.width(), 100, GRAY);
            Serial.println("Main layout drawn");
            current_text.set_text("");
            voltage_text.set_text("");
            power_text.set_text("");
            power_combined_text.set_text("");
            view_0.invalidate();
            view_1.invalidate();
        }

        float combined_voltage = 0;
        float combined_current = 0;
        float combined_power = 0;
        float combined_power_used = 0;
        for (int i = 0; i <= 1; i++)
        {
            PsuView *view = &view_0;
            Psu *psu = &psu_0;
            if (i == 1)
            {
                view = &view_1;
                psu = &psu_1;
            }
            if (psu->connected())
            {
                combined_voltage += psu->voltage_out();
                combined_current += psu->current_out();
                combined_power += psu->power_in();
                combined_power_used += psu->power_consumed();
                view->set_state(PsuState::normal);
                view->initial_draw();
                view->update_draw(format_precision(psu->voltage_out(), 2), format_precision(psu->current_out(), 1),
                                  format_precision(psu->temperature_internal(), 1),
                                  format_precision(psu->power_in(), 0));
            }
            else
            {
                view->set_state(PsuState::error);
                view->initial_draw();
            }
        }

        voltage_text.set_text(format_precision(combined_voltage, 2) + "V");
        current_text.set_text(format_precision(combined_current / 2, 1) + "A");
        int16_t x1, y1;
        uint16_t w, h;
        std::string text = format_precision(combined_power, 0) + "W";
        tft.setTextSize(7);
        tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
        power_text.set_text(text, (tft.width() - w) / 2, 175);

        text = format_precision(combined_power_used / 1000, 2) + "kWh";
        tft.setTextSize(1);
        tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
        power_combined_text.set_text(text, (tft.width() - w - 5), 230);
    }

    if (state == State::Detail)
    {
        if (last_state != state)
        {
            last_state = state;
            detail_view.invalidate();
            if (selected_psu == 0)
                draw_banner(tft, "DETAILS PSU 1");
            if (selected_psu == 1)
                draw_banner(tft, "DETAILS PSU 2");
            detail_view.initial_draw();
        }
        Psu *psu = &psu_0;
        if (selected_psu == 1)
            psu = &psu_1;
        if (psu->connected())
            detail_view.update_draw(
                format_precision(psu->voltage_in(), 2), format_precision(psu->voltage_out(), 2),
                format_precision(psu->current_in(), 1), format_precision(psu->current_out(), 1),
                format_precision(psu->power_in(), 0), format_precision(psu->power_out(), 0),
                format_precision(psu->temperature_intake(), 1), format_precision(psu->temperature_internal(), 1),
                format_precision(psu->fan_speed(), 0), format_precision(psu->seconds_since_startup(), 0),
                format_precision(psu->power_consumed(), 0));
    }

    if (state == State::Statistics)
    {
        constexpr int graph_width = 260;
        constexpr int graph_height = 150;
        constexpr int bars_count = 5;
        constexpr int bars_width = graph_width / bars_count;
        constexpr int x_pos = 40;

        static auto last_hour = 0;
        if (last_state != state || last_hour != consumption_per_hour.size())
        {
            last_hour = consumption_per_hour.size();
            last_state = state;
            draw_banner(tft, "STATISTIC");
            tft.fillRect(0, 25, tft.width(), tft.height(), WHITE);
            tft.fillRect(x_pos - 3, 200, graph_width + 3, 3, GRAY);
            tft.fillRect(x_pos - 3, 200, 3, -graph_height, GRAY);
            tft.setTextColor(BLACK);
            tft.setCursor(x_pos + 20, 210);
            tft.print("0");
            tft.setCursor(258, 210);
            tft.print("-5");
            tft.setCursor(3, 37);
            tft.print("2.4");
            tft.setTextSize(1);
            tft.setCursor(13, 52);
            tft.print("kw/h");
            for (int i = 0; i < bars_count; i++)
            {
                if (i >= consumption_per_hour.size())
                    break;
                // Maximum should be 2400Wh
                auto consumption = consumption_per_hour[consumption_per_hour.size() - i - 1];
                auto bar_height = (consumption / 2400.f) * graph_height;
                tft.fillRect(x_pos + bars_width * i, 200, bars_width, -bar_height, GRAY);
            }
        }

        static uint32_t flicker = 0;
        flicker++;
        auto consumption = consumption_per_hour.back();
        auto bar_height = (consumption / 2400.f) * graph_height;
        bar_height = std::max(1.f, bar_height);
        if (flicker % 2)
            tft.fillRect(x_pos, 200, bars_width, -bar_height, GRAY);
        else
            tft.fillRect(x_pos, 200, bars_width, -bar_height, RED);
    }
}

void collect_statistics_loop()
{
    static uint32_t last_time = 0;
    uint32_t time = millis();
    if (time < last_time)
        last_time = 0;
    if (time - last_time < 1000 * 30)
        return;
    last_time = time;

    auto hour = millis() / 1000 / 60 / 60;
    if (hour >= consumption_per_hour.size())
        consumption_per_hour.push_back(0);
    auto consumed = psu_0.power_consumed() + psu_1.power_consumed();
    for (int i = 0; i < hour; i++)
        consumed -= consumption_per_hour[i];
    Serial.print("Refresh statistics for hour ");
    Serial.println(hour);
    consumption_per_hour[hour] = consumed;
}

void touch_loop()
{
    auto p = ts.getPoint();
    // restore shared pins
    pinMode(YP, OUTPUT);
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);
    digitalWrite(XM, HIGH);
    if (p.z < 100)
        return;
    auto x = map(p.y, 913, 165, 0, tft.width());
    auto y = map(p.x, 172, 846, 0, tft.height());
    Serial.print("Touch on: ");
    Serial.print(x);
    Serial.print("x");
    Serial.print(y);
    Serial.print("|");
    Serial.println(p.z);
    if (state == State::Init)
    {
        // No touch operation
    }

    if (state == State::Overview)
    {
        if (x > 0 && x < 150 && y > 30 && y < 80)
        {
            selected_psu = 0;
            state = State::Detail;
            display_loop();
        }
        else if (x > 210 && x < 300 && y > 30 && y < 80)
        {
            selected_psu = 1;
            state = State::Detail;
            display_loop();
        }
        else if (y > 180)
        {
            state = State::Statistics;
            display_loop();
        }
    }

    if (state == State::Detail)
    {
        if (y > 0 && y < 40)
        {
            selected_psu = -1;
            state = State::Overview;
            display_loop();
        }
    }
    if (state == State::Statistics)
    {
        if (y > 0 && y < 40)
        {
            state = State::Overview;
            display_loop();
        }
    }
}

uint32_t output_error_time = 0;
bool output_error = false;

void enable_signal_loop()
{
    if (psu_0.connected() && psu_0.voltage_in() > 0 && psu_1.connected() && psu_1.voltage_in() > 0)
    {
        if (!output_error)
            digitalWrite(ENABLE_PIN, PinStatus::HIGH);

        // Make sure that  300ms after the outputs are enabled the PSUs outputs are active
        // If not switch the outputs off again.
        if (psu_0.voltage_out() <= 0 || psu_1.voltage_out() <= 0)
        {
            if (output_error_time == 0)
            {
                output_error_time = millis();
            }
            if (millis() - output_error_time > 300)
            {
                digitalWrite(ENABLE_PIN, PinStatus::LOW);
                output_error = true;
            }
        }
        else
        {
            output_error_time = 0;
        }
    }
    else
    {
        digitalWrite(ENABLE_PIN, PinStatus::LOW);
    }
}

void loop()
{
    uint32_t start = millis();
    enable_signal_loop();
    display_loop();
    touch_loop();
    collect_statistics_loop();

    uint32_t end = millis();
    // Serial.print("Main loop ms:");
    // Serial.println(std::to_string(end - start).c_str());
    delay(5);
}