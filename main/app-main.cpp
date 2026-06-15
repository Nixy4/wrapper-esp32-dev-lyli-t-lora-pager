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
#include "device/lilygo_t_lora_pager_keyboard.hpp"
#include "wrapper/freertos.hpp"
#include "wrapper/logger.hpp"

static const char* TAG = "App";

//! LVGL UI handles (accessed only under LvglPort lock)
//! ----------------------------------------------------------------------------------------
static lv_obj_t* g_key_label = nullptr;
static lv_obj_t* g_status_label = nullptr;

//! Board pointer for LVGL callbacks (set once before UI creation)
static wrapper::LilyGoLoraPager* g_board = nullptr;

//! Shutdown button click handler — runs inside LVGL task context
static void OnShutdownBtnClicked(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    ESP_LOGW(TAG, "Shutdown button pressed — cutting VSYS via BQ25896 BATFET_DIS");
    if (g_board)
        g_board->GetPmu().Shutdown();
}

static void CreateUi(wrapper::LilyGoLoraPager& board)
{
    wrapper::LvglPort& lvgl = board.GetLvglPort();
    lvgl.SetRotation(LV_DISPLAY_ROTATION_180);

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

    // ── Shutdown button ───────────────────────────────────────────────────────
    lv_obj_t* shutdown_btn = lv_button_create(scr);
    lv_obj_set_size(shutdown_btn, 120, 36);
    lv_obj_align(shutdown_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(shutdown_btn, lv_color_hex(0xCC2200), 0);
    lv_obj_add_event_cb(shutdown_btn, OnShutdownBtnClicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* shutdown_lbl = lv_label_create(shutdown_btn);
    lv_label_set_text(shutdown_lbl, "Power Off");
    lv_obj_set_style_text_font(shutdown_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(shutdown_lbl);

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

static void UpdateKeyLabel(wrapper::LilyGoLoraPager& board,
                           const wrapper::LilyGoLoRaPagerKeyEvent& ev)
{
    wrapper::LvglPort& lvgl = board.GetLvglPort();
    if (g_key_label == nullptr)
        return;
    if (!lvgl.Lock(50))
        return;

    char buf[48];
    if (ev.ch == '\n')
        snprintf(buf, sizeof(buf), "%s ENTER [%s] %s%s", ev.label, ev.pressed ? "DN" : "UP",
                 ev.cap_mode ? "[CAP]" : "", ev.sym_mode ? "[SYM]" : "");
    else if (ev.ch != '\0')
        snprintf(buf, sizeof(buf), "%s '%c' [%s] %s%s", ev.label, ev.ch, ev.pressed ? "DN" : "UP",
                 ev.cap_mode ? "[CAP]" : "", ev.sym_mode ? "[SYM]" : "");
    else
        snprintf(buf, sizeof(buf), "%s [%s] %s%s", ev.label, ev.pressed ? "DN" : "UP",
                 ev.cap_mode ? "[CAP]" : "", ev.sym_mode ? "[SYM]" : "");

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
        g_board = &board;
        CreateUi(board);
        UpdateStatusLabel(board, "I2C OK | SPI/LCD OK");

        // ── Audio codec ──────────────────────────────────────────────────────
        // ESP_LOGI(TAG, "==> InitAudio");
        // if (!board.InitAudio())
        // {
        //     ESP_LOGE(TAG, "InitAudio FAILED");
        //     UpdateStatusLabel(board, "I2C OK | LCD OK | Audio FAIL");
        // }
        // else
        // {
        //     ESP_LOGI(TAG, "I2S / ES8311 OK");
        //     UpdateStatusLabel(board, "I2C OK | LCD OK | Audio OK");
        // }

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
        static wrapper::Logger kb_logger("App", "KB", "Pager");
        wrapper::LilyGoLoRaPagerKeyboard kb(kb_logger, board.GetKeyboard());

        kb.SetKeyCallback(
            [&board](const wrapper::LilyGoLoRaPagerKeyEvent& ev)
            {
                ESP_LOGI(TAG, "Key %2u (%s) %s  [row=%u,col=%u]  ch='%c'  mode:%s%s", ev.code,
                         ev.label, ev.pressed ? "DN" : "UP", ev.row, ev.col, ev.ch ? ev.ch : '.',
                         ev.cap_mode ? "CAP " : "", ev.sym_mode ? "SYM" : "");

                // SYM + BS → 关机
                if (ev.pressed && ev.code == wrapper::LilyGoLoRaPagerKeyboard::kCodeBs &&
                    ev.sym_mode)
                {
                    ESP_LOGW(TAG, "SYM+BS: initiating shutdown via BQ25896 BATFET_DIS");
                    board.GetPmu().Shutdown();
                    return;
                }

                UpdateKeyLabel(board, ev);
            });

        ESP_LOGI(TAG, "Entering keyboard event loop");
        while (true)
        {
            kb.Poll();
            wrapper::Delay(20);
        }
    },
    nullptr,
    16384,
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
