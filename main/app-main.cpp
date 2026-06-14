#include "sdkconfig.h"
#include <cstdio>
#include <cstring>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

//! Board Driver
//! ----------------------------------------------------------------------------------------
#include "board/lilygo/t-lora-pager.hpp"
#include "wrapper/freertos.hpp"
#include "wrapper/logger.hpp"

static const char* TAG = "App";

//! Keyboard key-code → printable label table (4 rows × 10 cols = codes 1..40)
//! Layout follows the physical key positions of the T-LoraPager keyboard.
//! Code = row * num_cols + col + 1  (row=0..3, col=0..9)
//! ----------------------------------------------------------------------------------------
static constexpr int kKeyRows = 4;
static constexpr int kKeyCols = 10;

static const char* KeyLabel(uint8_t code)
{
    // clang-format off
    static const char* kTable[kKeyRows * kKeyCols + 1] = {
        "?",                                                    // 0 = invalid
        "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "0",    // row 0: 1-10
        "Q",  "W",  "E",  "R",  "T",  "Y",  "U",  "I",  "O",  "P",    // row 1: 11-20
        "A",  "S",  "D",  "F",  "G",  "H",  "J",  "K",  "L",  "BS",   // row 2: 21-30
        "ALT","Z",  "X",  "C",  "V",  "B",  "N",  "M", "SPC", "ENT",  // row 3: 31-40
    };
    // clang-format on
    if (code == 0 || code > kKeyRows * kKeyCols)
        return "?";
    return kTable[code];
}

//! LVGL UI handles (accessed only under LvglPort lock)
//! ----------------------------------------------------------------------------------------
static lv_obj_t* g_key_label = nullptr;
static lv_obj_t* g_status_label = nullptr;

static void CreateUi(wrapper::LilyGoLoraPager& board)
{
    wrapper::LvglPort& lvgl = board.GetLvglPort();

    if (!lvgl.Lock(500))
    {
        ESP_LOGE(TAG, "LVGL lock timeout during UI creation");
        return;
    }

    lv_obj_t* scr = lv_scr_act();

    // ── Title bar ────────────────────────────────────────────────────────────
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "LilyGo T-LoraPager");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // ── Separator ────────────────────────────────────────────────────────────
    lv_obj_t* sep = lv_obj_create(scr);
    lv_obj_set_size(sep, 460, 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 28);

    // ── Status label (init result) ────────────────────────────────────────────
    g_status_label = lv_label_create(scr);
    lv_label_set_text(g_status_label, "Initializing...");
    lv_obj_set_style_text_font(g_status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(g_status_label, LV_ALIGN_TOP_LEFT, 10, 40);

    // ── Key event area ────────────────────────────────────────────────────────
    lv_obj_t* key_title = lv_label_create(scr);
    lv_label_set_text(key_title, "Last key:");
    lv_obj_set_style_text_font(key_title, &lv_font_montserrat_14, 0);
    lv_obj_align(key_title, LV_ALIGN_TOP_LEFT, 10, 100);

    g_key_label = lv_label_create(scr);
    lv_label_set_text(g_key_label, "--");
    lv_obj_set_style_text_font(g_key_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_key_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(g_key_label, LV_ALIGN_TOP_LEFT, 10, 120);

    lvgl.Unlock();
}

static void UpdateStatusLabel(wrapper::LilyGoLoraPager& board, const char* text)
{
    wrapper::LvglPort& lvgl = board.GetLvglPort();
    if (g_status_label == nullptr)
        return;
    if (!lvgl.Lock(100))
        return;
    lv_label_set_text(g_status_label, text);
    lvgl.Unlock();
}

static void UpdateKeyLabel(wrapper::LilyGoLoraPager& board, uint8_t code, bool pressed)
{
    wrapper::LvglPort& lvgl = board.GetLvglPort();
    if (g_key_label == nullptr)
        return;
    if (!lvgl.Lock(50))
        return;

    char buf[32];
    snprintf(buf, sizeof(buf), "%s [%s]", KeyLabel(code), pressed ? "DN" : "UP");
    lv_label_set_text(g_key_label, buf);

    lvgl.Unlock();
}

//! Main board initialisation & test task
//! ----------------------------------------------------------------------------------------
wrapper::Task board_init(
    "BoardInit",
    [](void*)
    {
        wrapper::LilyGoLoraPager& board = wrapper::LilyGoLoraPager::GetInstance();

        // ── Core bus + IO expander + PMU ─────────────────────────────────────
        ESP_LOGI(TAG, "==> InitCoreBusAndIoExpander");
        if (!board.InitCoreBusAndIoExpander())
        {
            ESP_LOGE(TAG, "InitCoreBusAndIoExpander FAILED");
        }
        else
        {
            ESP_LOGI(TAG, "I2C / XL9555 / BQ25896 OK");
        }

        // ── Display + LVGL ───────────────────────────────────────────────────
        ESP_LOGI(TAG, "==> InitDisplay");
        if (!board.InitDisplay())
        {
            ESP_LOGE(TAG, "InitDisplay FAILED");
        }
        else
        {
            ESP_LOGI(TAG, "ST7796 / LVGL OK (480x222)");
        }

        // Build UI now that LVGL is running
        CreateUi(board);
        UpdateStatusLabel(board, "I2C OK | SPI/LCD OK");

        // ── Audio codec ──────────────────────────────────────────────────────
        ESP_LOGI(TAG, "==> InitAudio");
        if (!board.InitAudio())
        {
            ESP_LOGE(TAG, "InitAudio FAILED");
            UpdateStatusLabel(board, "I2C OK | LCD OK | Audio FAIL");
        }
        else
        {
            ESP_LOGI(TAG, "I2S / ES8311 OK");
            UpdateStatusLabel(board, "I2C OK | LCD OK | Audio OK");
        }

        // ── Keyboard matrix ──────────────────────────────────────────────────
        ESP_LOGI(TAG, "==> InitKeyboard");
        if (!board.InitKeyboard())
        {
            ESP_LOGE(TAG, "InitKeyboard FAILED");
            UpdateStatusLabel(board, "I2C OK | LCD OK | Audio OK | KB FAIL");
        }
        else
        {
            ESP_LOGI(TAG, "TCA8418 4x10 OK");
            UpdateStatusLabel(board, "All peripherals OK — press any key");
        }

        // ── Keyboard event loop ──────────────────────────────────────────────
        wrapper::Tca8418& kb = board.GetKeyboard();
        ESP_LOGI(TAG, "Entering keyboard event loop");

        while (true)
        {
            if (kb.EventAvailable())
            {
                uint8_t code = 0;
                bool pressed = false;

                while (kb.GetKeyEvent(code, pressed))
                {
                    ESP_LOGI(TAG, "Key %u (%s) %s", code, KeyLabel(code),
                             pressed ? "pressed" : "released");
                    UpdateKeyLabel(board, code, pressed);
                }
            }
            wrapper::Delay(20);
        }
    },
    nullptr,
    8192,
    5);

//! Application entry point
//! ----------------------------------------------------------------------------------------
extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    board_init.Create();
}
