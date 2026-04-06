// gcbot or GameCube bot
// Record and play DTM files on a GameCube.
// https://github.com/jiinurppa/gcbot
#include "st7735red.hpp"
#include "dtm_reader.hpp"
#include "dtm_writer.hpp"
#include "GamecubeConsole.hpp"
#include "GamecubeController.hpp"
#include "gamecube_definitions.h"
#include <cstdint>
#include <string>
#include <pico/stdlib.h>
#include <pico/float.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

const std::string PROJECT_TITLE = "GAMECUBE BOT V1.0";

enum Mode
{
    Passthrough = 0,
    DTM_Playback,
    DTM_Recording
};

Mode mode = Passthrough;
void read_mode();

queue_t input_queue;
uint64_t inputs_sent = 0;
auto report = default_gc_report;

bool paused = false;
bool prefill_finished = false;
bool recording_finished = false;

void core1_passthrough();
void core1_dtm_reader();
void core1_dtm_writer();

inline void dtm_to_report(const uint8_t* bytes, gc_report_t& report);
inline void report_to_dtm(const gc_report_t& report, uint8_t* bytes);

int main()
{
    set_sys_clock_khz(130'000, true);
    stdio_init_all();
    read_mode();

    if (mode == Passthrough)
    {
        // Timing measurement pins
        gpio_init(28);
        gpio_set_dir(28, GPIO_OUT);
        gpio_put(28, false);
        gpio_init(27);
        gpio_set_dir(27, GPIO_OUT);
        gpio_put(27, false);
        multicore_launch_core1(core1_passthrough);
    }
    else if (mode == DTM_Playback)
    {
        constexpr uint INPUT_BUFER_SIZE = 512;
        queue_init(&input_queue, sizeof(gc_report_t), INPUT_BUFER_SIZE);
        multicore_launch_core1(core1_dtm_reader);
    }
    else if (mode == DTM_Recording)
    {
        constexpr uint INPUT_BUFER_SIZE = 512;
        queue_init(&input_queue, sizeof(gc_report_t), INPUT_BUFER_SIZE);
        multicore_launch_core1(core1_dtm_writer);
    }

    // Setup Joybuses
    constexpr uint8_t PICO_TO_GC_DATA_PIN = 2;
    GamecubeConsole console(PICO_TO_GC_DATA_PIN , pio0);
    constexpr uint8_t CONTROLLER_TO_PICO_DATA_PIN = 5;
    constexpr uint CONTROLLER_POLLING_RATE = 120;
    GamecubeController controller(CONTROLLER_TO_PICO_DATA_PIN, CONTROLLER_POLLING_RATE, pio1);

    // Wait for prefill
    if (mode == DTM_Playback)
    {
        while (!prefill_finished) tight_loop_contents();
    }

    // Wait for console to boot
    while (!console.Detect()) busy_wait_ms(20);

    // Wait for GameCube startup animation to finish
    busy_wait_ms(8000);

    while (mode == Passthrough)
    {
        controller.Poll(&report, false);
        gpio_put(28, true);
        console.WaitForPoll();
        gpio_put(28, false);
        gpio_put(27, true);
        console.SendReport(&report);
        gpio_put(27, false);
    }

    while (mode == DTM_Recording)
    {
        controller.Poll(&report, false);
        console.WaitForPoll();
        console.SendReport(&report);

        if (!paused && !recording_finished)
        {
            queue_add_blocking(&input_queue, &report);
        }

        // Start + Z + Y ends recording
        // Start + Z + A resumes recording
        // Start + Z + B pauses recording
        if (!recording_finished && report.start && report.z)
        {
            if (report.y)
            {
                recording_finished = true;
            }
            else if (report.a)
            {
                paused = false;
            }
            else if (report.b)
            {
                paused = true;
            }
        }
    }

    while (mode == DTM_Playback)
    {
        controller.Poll(&report, false);
        console.WaitForPoll();

        // Send inputs from DTM file
        if (queue_try_remove(&input_queue, &report))
        {
            console.SendReport(&report);
            ++inputs_sent;
        }
        // Send inputs from controller after playback
        else
        {
            console.SendReport(&report);
        }
    }
}

void read_mode()
{
    gpio_init(6);
	gpio_set_dir(6, GPIO_IN);
	gpio_pull_up(6);

    gpio_init(7);
	gpio_set_dir(7, GPIO_IN);
	gpio_pull_up(7);

    gpio_init(8);
	gpio_set_dir(8, GPIO_IN);
	gpio_pull_up(8);

    if (gpio_get(6) == false)
    {
        mode = Passthrough;
    }
    else if (gpio_get(7) == false)
    {
        mode = DTM_Playback;
    }
    else if (gpio_get(8) == false)
    {
        mode = DTM_Recording;
    }
}

void core1_passthrough()
{
    ST7735Red display;
    auto BG_COLOR = display.rgb_to_color(83, 84, 134);
    display.clear_buffer(BG_COLOR);
    display.draw_text(PROJECT_TITLE, 2, 2);
    display.draw_text("PASSTHROUGH MODE", 2, 34);
    display.write_buffer_to_display();

    auto current_input = default_gc_report;
    uint8_t heartbeat_counter = 1;
    std::string inputs(10, ' ');

    static const uint8_t button_bg[12][2] =
    {
        // Top half
        {0x07,0xE0},{0x0F,0xF0},{0x1F,0xF8},
        {0x3F,0xFC},{0x3F,0xFC},{0x3F,0xFC},
        // Bottom half
        {0x3F,0xFC},{0x3F,0xFC},{0x3F,0xFC},
        {0x1F,0xF8},{0x0F,0xF0},{0x07,0xE0}
    };

    uint8_t buttons_y_offset = display.height() - 22;

    while (true)
    {
        display.draw_horizontal_line(0, display.height() - 22, display.width() - 1, 22, BG_COLOR);

        // Draw heartbeat
        if (heartbeat_counter > 30)
        {
            display.draw_horizontal_line(2, display.height() - 3, 4, 2, display.COLOR_YELLOW);
        }

        heartbeat_counter = heartbeat_counter == 60 ? 1 : ++heartbeat_counter;

        // Draw buttons
        current_input = report;
        inputs[0] = current_input.a ? 'a' : ' ';
        inputs[1] = current_input.b ? 'b' : ' ';
        inputs[2] = current_input.x ? 'x' : ' ';
        inputs[3] = current_input.y ? 'y' : ' ';
        inputs[4] = current_input.z ? 'z' : ' ';
        inputs[5] = current_input.start ? 's' : ' ';
        inputs[6] = current_input.dpad_up ? 'u' : ' ';
        inputs[7] = current_input.dpad_down ? 'd' : ' ';
        inputs[8] = current_input.dpad_left ? 'l' : ' ';
        inputs[9] = current_input.dpad_right ? 'r' : ' ';
        
        for (uint8_t i = 0; i < inputs.length(); ++i)
        {
            if (inputs[i] == ' ')
            {
                continue;
            }

            // Draw button circle backgrounds
            uint16_t color;
            if (i == 0) color = 0x04C0; // A
            else if (i == 1) color = 0xC800; // B
            else if (i == 4) color = 0x8817; // Z
            else color = display.COLOR_GRAY;

            uint8_t x = 16 * i;

            for (uint8_t y = 0; y < 12; ++y)
            {
                for (uint8_t j = 0; j < 8; ++j)
                {
                    // Left side
                    if ((button_bg[y][1] >> j) & 1)
                    {
                        display.draw_pixel(x + j, y + buttons_y_offset, color);
                    }

                    // Right side
                    if ((button_bg[y][0] >> j) & 1)
                    {
                        display.draw_pixel(x + j + 8, y + buttons_y_offset, color);
                    }
                }
            }

            // Draw button label
            display.draw_character(inputs[i], x + 4, display.height() - 19, display.COLOR_WHITE);
        }

        display.write_buffer_to_display(0, display.height() - 22, display.width() - 1);
    }
}

void core1_dtm_reader()
{
    // Light up on-board LED during prefill
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, true);

    DTMReader dtm_reader;
    bool read_error = dtm_reader.error_message() != "";
    
    uint8_t input_bytes[dtm_reader.INPUT_SIZE_BYTES];
    auto input_from_dtm = default_gc_report;

    uint64_t inputs_read = 0;
    auto timestamp_start = get_absolute_time();

    // Initial read until queue is full or all inputs are buffered
    while (!read_error && !queue_is_full(&input_queue))
    {
        read_error = !dtm_reader.get_next_input(input_bytes);

        if (!read_error)
        {
            dtm_to_report(input_bytes, input_from_dtm);
            queue_try_add(&input_queue, &input_from_dtm);
            ++inputs_read;
        }

        if (dtm_reader.all_inputs_read())
        {
            break;
        }
    }

    auto timestamp_end = get_absolute_time();
    gpio_put(25, false);

    ST7735Red display;
    auto BG_COLOR = display.rgb_to_color(83, 84, 134);
    display.clear_buffer(BG_COLOR);
    display.draw_text(PROJECT_TITLE, 2, 2);

    if (!read_error)
    {
        display.draw_text("DTM FILE OK", 2, 34);
        auto str = "PREFILL COUNT " + std::to_string(inputs_read);
        display.draw_text(str, 2, 44);
        auto duration_ms = absolute_time_diff_us(timestamp_start, timestamp_end) / 1000;
        str = "TOOK " + std::to_string(duration_ms) + " MS";
        display.draw_text(str, 2, 54);
    }

    display.write_buffer_to_display();
    auto next_screen_update = make_timeout_time_ms(500);
    std::string progress_text = "";
    bool blink_state = false;
    prefill_finished = true;

    while (true)
    {
        if (read_error ||
            (dtm_reader.all_inputs_read() && queue_is_empty(&input_queue)))
        {
            break;
        }

        // The buffer should never be empty here
        if (queue_is_empty(&input_queue) && !dtm_reader.all_inputs_read())
        {
            display.draw_text("BUFFER EXHAUSTED", 2, 70, display.COLOR_RED);
            break;
        }

        // Fill buffer
        if (!queue_is_full(&input_queue) && !dtm_reader.all_inputs_read())
        {
            read_error = !dtm_reader.get_next_input(input_bytes);

            if (!read_error)
            {
                dtm_to_report(input_bytes, input_from_dtm);
                queue_try_add(&input_queue, &input_from_dtm);
            }
        }

        // Update display
        if (next_screen_update < get_absolute_time())
        {
            display.draw_horizontal_line(0, display.height() - 20, 96, 20, BG_COLOR);
            display.draw_text("PLAY", 2, display.height() - 20, display.COLOR_GREEN);

            if (blink_state)
            {
                display.draw_text(">", 44, display.height() - 20, display.COLOR_GREEN);
            }

            blink_state = !blink_state;

            // Display how many inputs from DTM have been sent to GC
            auto inputs_read = uint642float(inputs_sent);
            auto inputs_total = uint642float(dtm_reader.input_count());
            auto progressf = 100 * inputs_read / inputs_total;
            auto progressi = float2int_z(progressf);
            progress_text = std::to_string(progressi) + "%";
            display.draw_text(progress_text, 64, display.height() - 20, display.COLOR_WHITE);

            display.write_buffer_to_display(0, display.height() - 20, 96);
            next_screen_update = make_timeout_time_ms(500);
        }
    }

    display.draw_horizontal_line(0, display.height() - 20, display.width() - 1, 20, BG_COLOR);

    if (read_error)
    {
        display.draw_text(dtm_reader.error_message(), 2, display.height() - 20, display.COLOR_RED);
    }
    else if (dtm_reader.all_inputs_read())
    {
        display.draw_text("FINISHED", 2, display.height() - 20);
    }

    display.write_buffer_to_display();

    // Done
    while (true) tight_loop_contents();
}

void core1_dtm_writer()
{
    DTMWriter dtm_writer;
    uint8_t input_bytes[dtm_writer.INPUT_SIZE_BYTES];
    auto input_to_dtm = default_gc_report;

    ST7735Red display;
    auto BG_COLOR = display.rgb_to_color(83, 84, 134);
    display.clear_buffer(BG_COLOR);
    display.draw_text(PROJECT_TITLE, 2, 2);
    display.draw_text("RECORDING MODE", 2, 34);

    display.write_buffer_to_display();

    bool write_error = dtm_writer.error_message() != "";
    bool blink_state = false;
    auto next_screen_update = make_timeout_time_ms(500);

    while (true)
    {
        if (write_error ||
            (recording_finished && queue_is_empty(&input_queue)))
        {
            break;
        }

        // Write input
        if (queue_try_remove(&input_queue, &input_to_dtm))
        {
            report_to_dtm(input_to_dtm, input_bytes);
            write_error = !dtm_writer.write_input(input_bytes);
        }

        // Update display
        if (next_screen_update < get_absolute_time())
        {
            display.draw_horizontal_line(0, display.height() - 20, 64, 20, BG_COLOR);

            if (paused)
            {
                display.draw_text("PAUSE", 2, display.height() - 20, display.COLOR_WHITE);
            }
            else
            {
                display.draw_text("REC", 2, display.height() - 20, display.COLOR_RED);
            }

            if (blink_state)
            {
                if (paused)
                {
                    display.draw_text("|", 48, display.height() - 20, display.COLOR_WHITE);
                }
                else
                {
                    display.draw_text("o", 34, display.height() - 20, display.COLOR_RED);
                }
            }

            blink_state = !blink_state;
            display.write_buffer_to_display(0, display.height() - 20, 64);
            next_screen_update = make_timeout_time_ms(500);            
        }
    }

    display.draw_horizontal_line(0, display.height() - 20, 64, 20, BG_COLOR);

    if (recording_finished)
    {
        write_error = !dtm_writer.finalize_file();
    }

    if (write_error)
    {
        display.draw_text(dtm_writer.error_message(), 2, display.height() - 20, display.COLOR_RED);
    }
    else
    {
        display.draw_text("DONE", 2, display.height() - 20);
    }

    display.write_buffer_to_display(0, display.height() - 20);

    // Done
    while (true) tight_loop_contents();
}

// Convert DTM to gc_report_t
// Reference: https://tasvideos.org/EmulatorResources/Dolphin/DTM
void dtm_to_report(const uint8_t* bytes, gc_report_t& report)
{
    constexpr uint8_t DTM_BUTTON_START = 0x01;
    constexpr uint8_t DTM_BUTTON_A = 0x02;
    constexpr uint8_t DTM_BUTTON_B = 0x04;
    constexpr uint8_t DTM_BUTTON_X = 0x08;
    constexpr uint8_t DTM_BUTTON_Y = 0x10;
    constexpr uint8_t DTM_BUTTON_Z = 0x20;
    constexpr uint8_t DTM_BUTTON_UP = 0x40;
    constexpr uint8_t DTM_BUTTON_DOWN = 0x80;
    constexpr uint8_t DTM_BUTTON_LEFT = 0x01;
    constexpr uint8_t DTM_BUTTON_RIGHT = 0x02;
    constexpr uint8_t DTM_BUTTON_L = 0x04;
    constexpr uint8_t DTM_BUTTON_R = 0x08;
    // Digital
    report.start      = (bytes[0] & DTM_BUTTON_START) != 0;
    report.a          = (bytes[0] & DTM_BUTTON_A) != 0;
    report.b          = (bytes[0] & DTM_BUTTON_B) != 0;
    report.x          = (bytes[0] & DTM_BUTTON_X) != 0;
    report.y          = (bytes[0] & DTM_BUTTON_Y) != 0;
    report.z          = (bytes[0] & DTM_BUTTON_Z) != 0;
    report.dpad_up    = (bytes[0] & DTM_BUTTON_UP) != 0;
    report.dpad_down  = (bytes[0] & DTM_BUTTON_DOWN) != 0;
    report.dpad_left  = (bytes[1] & DTM_BUTTON_LEFT) != 0;
    report.dpad_right = (bytes[1] & DTM_BUTTON_RIGHT) != 0;
    report.r          = (bytes[1] & DTM_BUTTON_R) != 0;
    report.l          = (bytes[1] & DTM_BUTTON_L) != 0;
    // Analog
    report.l_analog = bytes[2];
    report.r_analog = bytes[3];
    report.stick_x = bytes[4];
    report.stick_y = bytes[5];
    report.cstick_x = bytes[6];
    report.cstick_y = bytes[7];
}

// Convert gc_report_t to DTM
void report_to_dtm(const gc_report_t& report, uint8_t* bytes)
{
    // Digital
    bytes[0] = uint8_t(report.start ? 1 : 0);
    bytes[0] |= uint8_t(report.a ? 1 : 0) << 1;
    bytes[0] |= uint8_t(report.b ? 1 : 0) << 2;
    bytes[0] |= uint8_t(report.x ? 1 : 0) << 3;
    bytes[0] |= uint8_t(report.y ? 1 : 0) << 4;
    bytes[0] |= uint8_t(report.z ? 1 : 0) << 5;
    bytes[0] |= uint8_t(report.dpad_up ? 1 : 0) << 6;
    bytes[0] |= uint8_t(report.dpad_down ? 1 : 0) << 7;
    bytes[1] = 0x40;
    bytes[1] |= uint8_t(report.dpad_left ? 1 : 0);
    bytes[1] |= uint8_t(report.dpad_right ? 1 : 0) << 1;
    bytes[1] |= uint8_t(report.l ? 1 : 0) << 2;
    bytes[1] |= uint8_t(report.r ? 1 : 0) << 3;
    // Analog
    bytes[2] = report.l_analog;
    bytes[3] = report.r_analog;
    bytes[4] = report.stick_x;
    bytes[5] = report.stick_y;
    bytes[6] = report.cstick_x;
    bytes[7] = report.cstick_y;
}
