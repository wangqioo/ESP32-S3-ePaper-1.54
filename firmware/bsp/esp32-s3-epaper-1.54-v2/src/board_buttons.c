#include "board_buttons.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "multi_button.h"

#define set_bit_button(x) ((uint32_t)(0x01) << (x))

EventGroupHandle_t boot_groups;
EventGroupHandle_t pwr_groups;

static const char *TAG = "board_buttons";

static Button s_boot_button;
static Button s_pwr_button;
static esp_timer_handle_t s_button_timer;

enum {
    BOOT_BUTTON_ID = 1,
    PWR_BUTTON_ID = 2,
    BUTTON_ACTIVE_LEVEL = 0,
};

static void on_boot_single_click(Button *btn_handle);
static void on_boot_longpress_press(Button *btn_handle);
static void on_boot_pressup_press(Button *btn_handle);
static void on_pwr_single_click(Button *btn_handle);
static void on_pwr_double_press(Button *btn_handle);
static void on_pwr_longpress_press(Button *btn_handle);
static void on_pwr_pressup_press(Button *btn_handle);

static void clock_task_callback(void *arg)
{
    (void)arg;
    button_ticks();
}

static uint8_t read_button_gpio(uint8_t button_id)
{
    switch (button_id) {
    case BOOT_BUTTON_ID:
        return gpio_get_level(BOARD_BOOT_BUTTON_PIN);
    case PWR_BUTTON_ID:
        return gpio_get_level(BOARD_PWR_BUTTON_PIN);
    default:
        return 1;
    }
}

static esp_err_t configure_button_gpio(void)
{
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << BOARD_BOOT_BUTTON_PIN) | (1ULL << BOARD_PWR_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&gpio_conf);
}

esp_err_t board_buttons_init(void)
{
    if (s_button_timer != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(configure_button_gpio(), TAG, "button gpio config failed");

    if (boot_groups == NULL) {
        boot_groups = xEventGroupCreate();
        ESP_RETURN_ON_FALSE(boot_groups != NULL, ESP_ERR_NO_MEM, TAG, "boot event group alloc failed");
    }
    if (pwr_groups == NULL) {
        pwr_groups = xEventGroupCreate();
        ESP_RETURN_ON_FALSE(pwr_groups != NULL, ESP_ERR_NO_MEM, TAG, "pwr event group alloc failed");
    }

    button_init(&s_boot_button, read_button_gpio, BUTTON_ACTIVE_LEVEL, BOOT_BUTTON_ID);
    button_attach(&s_boot_button, BTN_SINGLE_CLICK, on_boot_single_click);
    button_attach(&s_boot_button, BTN_LONG_PRESS_START, on_boot_longpress_press);
    button_attach(&s_boot_button, BTN_PRESS_UP, on_boot_pressup_press);

    button_init(&s_pwr_button, read_button_gpio, BUTTON_ACTIVE_LEVEL, PWR_BUTTON_ID);
    button_attach(&s_pwr_button, BTN_SINGLE_CLICK, on_pwr_single_click);
    button_attach(&s_pwr_button, BTN_DOUBLE_CLICK, on_pwr_double_press);
    button_attach(&s_pwr_button, BTN_LONG_PRESS_START, on_pwr_longpress_press);
    button_attach(&s_pwr_button, BTN_PRESS_UP, on_pwr_pressup_press);

    esp_timer_create_args_t timer_args = {
        .callback = clock_task_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "button_tick",
        .skip_unhandled_events = false,
    };

    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_button_timer), TAG, "button timer create failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_button_timer, 1000 * TICKS_INTERVAL),
                        TAG,
                        "button timer start failed");

    button_start(&s_pwr_button);
    button_start(&s_boot_button);
    return ESP_OK;
}

uint8_t board_button_boot_repeat_count(void)
{
    return button_get_repeat_count(&s_boot_button);
}

uint8_t board_button_pwr_repeat_count(void)
{
    return button_get_repeat_count(&s_pwr_button);
}

static void on_boot_single_click(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(boot_groups, set_bit_button(0));
}

static void on_boot_longpress_press(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(boot_groups, set_bit_button(1));
}

static void on_boot_pressup_press(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(boot_groups, set_bit_button(2));
}

static void on_pwr_single_click(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(pwr_groups, set_bit_button(0));
}

static void on_pwr_double_press(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(pwr_groups, set_bit_button(1));
}

static void on_pwr_longpress_press(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(pwr_groups, set_bit_button(2));
}

static void on_pwr_pressup_press(Button *btn_handle)
{
    (void)btn_handle;
    xEventGroupSetBits(pwr_groups, set_bit_button(3));
}
