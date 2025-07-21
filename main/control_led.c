#include "control_led.h"
#include "sdkconfig.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LED_control"; // TAG for debug

#ifdef CONFIG_BLINK_LED_STRIP

led_strip_handle_t rgb_led;
led_strip_handle_t led_strip;

/// @brief rgb to hsv according to https://en.wikipedia.org/wiki/HSL_and_HSV
/// @param r  red 0 to 255
/// @param g  green 0 to 255
/// @param b  blue 0 to 255
/// @param h  pointer to the hue 0 to 360
/// @param s  pointer to the saturation 0 to 100
/// @param v  pointer to the value 0 to 100
void led_strip_rgb2hsv(uint32_t r, uint32_t g, uint32_t b, uint32_t *h, uint32_t *s, uint32_t *v)
{
    r %= 255;
    g %= 255;
    b %= 255;
    *v = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    uint32_t xmin = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    uint32_t chroma = *v - xmin;
    if(chroma == 0) *h = 0;
    else {
        if(*v == r) {
            float hue = 60.0 * ((float)g - (float)b) / (float)chroma;
            *h = (hue > 0) ? (uint32_t)hue : (uint32_t)(360. + hue);
        } else if(*v == g) {
            float hue = 60.0 * (((float)b - (float)r) / (float)chroma + 2.);
            *h = (uint32_t)hue;
        } else if(*v == b) {
            float hue = 60.0 * (((float)r - (float)g) / (float)chroma + 4.);
            *h = (uint32_t)hue;
        }
    }
    if(*v == 0) *s = 0;
    else *s = (100 * chroma) / *v;
    *v = (100 * *v) / 255;
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void blink_led(void)
{
    static uint32_t hue = 0;
    uint32_t red, green, blue;
    /* If the addressable LED is enabled */
    if (led_state)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_hsv2rgb(hue, 100, LED_BRIGHTNESS, &red, &green, &blue);
        ESP_LOGI(TAG,"red:%02lx green:%02lx blue:%02lx", red, green, blue);
        led_strip_set_pixel(rgb_led, 0, red, green, blue);
        /* Refresh the strip to send data */
        led_strip_refresh(rgb_led);
        hue += 6;
        hue %= 360;
    }
    else
    {
        /* Set all LED off to clear all pixels */
        led_strip_clear(rgb_led);
    }
}

void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t rgb_config = {
        .strip_gpio_num = BLED_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_config_t strip_config = {
        .strip_gpio_num = STRIP_GPIO,
        .max_leds = 8, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_rgb_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&rgb_config, &rmt_rgb_config, &rgb_led));
    led_strip_rmt_config_t rmt_strip_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_strip_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&rgb_config, &spi_config, &rgb_led));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(rgb_led);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLED_GPIO, led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLED_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif
