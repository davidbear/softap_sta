#include "control_led.h"
#include "sdkconfig.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LED_control"; // TAG for debug

#ifdef CONFIG_BLINK_LED_STRIP

struct hsv_struct {
    // Union allows 'hues' and the struct below to share memory
    union {
        uint16_t hues[3];      // Access as array: hues[0], hues[1], hues[2]
        
        // Anonymous struct keeps the variables accessible directly
        struct {
            uint16_t sec_hue;  // Maps to hues[0]
            uint16_t min_hue;  // Maps to hues[1]
            uint16_t hour_hue; // Maps to hues[2]
        };
    };
    uint8_t led_no;
    unsigned char bcd : 1;
    unsigned char reverse : 1;
    unsigned char clockwise : 1;
    unsigned char std_clr : 1;
};

led_strip_handle_t rgb_led;
led_strip_handle_t strip_leds;

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

void toggle_rgb_led(void)
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
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_strip_config, &strip_leds));
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
    led_strip_clear(strip_leds);
}

// Helper: Converts integer to BCD (assuming this implementation)
// If you have a different one, swap this out.
uint8_t to_bcd(int val) {
    return ((val / 10) << 4) | (val % 10);
}

// Returns the INDEX (0, 1, or 2) of the smallest value
uint8_t get_min_hue_index(const uint16_t hues[3]) {
    if (hues[0] < hues[1]) {
        return (hues[0] < hues[2]) ? 0 : 2;
    } else {
        return (hues[1] < hues[2]) ? 1 : 2;
    }
}


// Helper: Replicates the exact wrapping logic from the original code.
// Note: argument order matters! v1 is the primary value compared against v2.
static uint16_t blend(uint16_t v1, uint16_t v2, bool clockwise) {
    if (clockwise) {
        // Original: if(v1 > v2) avg else wrap
        if (v1 > v2) return (v1 + v2) / 2;
        
        uint32_t temp = (uint32_t)v1 + 360 + v2;
        uint16_t res = temp / 2;
        return (res > 360) ? res - 360 : res;
    } else {
        // Original: if(v2 > v1) avg else wrap
        if (v2 > v1) return (v1 + v2) / 2;
        
        uint32_t temp = (uint32_t)v2 + 360 + v1;
        uint16_t res = temp / 2;
        return (res > 360) ? res - 360 : res;
    }
}

void make_hsv(const struct tm *timeinfo, const struct hsv_struct *hsv_info, uint16_t *h, uint8_t *s, uint8_t *v)
{
    // --- 1. Setup ---
    uint8_t led_no = hsv_info->led_no;
    bool bcd_mode  = hsv_info->bcd;
    bool reverse   = hsv_info->reverse;
    bool clockwise = hsv_info->clockwise;

    int tm_sec  = timeinfo->tm_sec;
    int tm_min  = timeinfo->tm_min;
    int tm_hour = timeinfo->tm_hour;

    // --- 2. BCD Handling & Dead Pixels ---
    if (bcd_mode) {
        // Handle "unused" BCD bits (dimmed white)
        if ((!reverse && led_no == 4) || (reverse && led_no == (N_LEDS - 5))) {
            *h = 0; 
            *s = 0; 
            *v = BRIGHTNESS / 10; 
            return;
        }
        tm_sec  = to_bcd(tm_sec);
        tm_min  = to_bcd(tm_min);
        tm_hour = to_bcd(tm_hour);
    }

    // --- 3. Mask Calculation ---
    uint16_t mask;
    if (reverse) {
        if (bcd_mode) {
             mask = (led_no > (N_LEDS - 5)) ? (1 << (N_LEDS - led_no - 1)) 
                                 : (1 << (N_LEDS - led_no - 2));
        } else {
             mask = (1 << (N_LEDS - led_no - 1));
        }
    } else {
        if (bcd_mode && led_no > 4) {
            mask = (1 << (led_no - 1));
        } else {
            mask = (1 << led_no);
        }
    }

    // --- 4. Determine Active Components ---
    bool sec_on  = (tm_sec & mask);
    bool min_on  = (tm_min & mask);
    bool hour_on = (tm_hour & mask);

    // --- 5. Color Logic ---
    
    // Default assumptions (Active = Bright/Sat). 
    // We only zero these if specific "Off" or "White" conditions are met.
    // (This mimics original behavior where v/s are untouched in active branches)
    // NOTE: Depending on your loop, you might want to initialize *s=255, *v=BRIGHTNESS here.
    // I will not force them, to behave exactly like original, 
    // but I will ensure the OFF/WHITE cases explicitly set them.

    if (sec_on) {
        // [Sec = ON]
        *h = hsv_info->sec_hue; // Start with Sec hue

        if (min_on) {
            // [Sec = ON, Min = ON] -> Blend Min+Sec
            
            if (hour_on) {
                // [Sec = ON, Min = ON, Hour = ON] -> White
                *h = 0;
                *s = 0; 
                // Note: Original code does NOT set *v=0 here, so we leave it alone.
            }
            
            // Blend is applied even if Hour is ON (overwriting *h, but s is 0 so it looks white)
            // Original: if(clockwise) { if(min > sec) ... }
            // Argument 1 is MIN, Argument 2 is SEC
            *h = blend(hsv_info->min_hue, hsv_info->sec_hue, clockwise);
        } 
        else {
            // [Sec = ON, Min = OFF]
            if (hour_on) {
                // [Sec = ON, Min = OFF, Hour = ON] -> Blend Sec+Hour
                // Original: if(clockwise) { if(sec > hour) ... }
                // Argument 1 is SEC, Argument 2 is HOUR
                *h = blend(hsv_info->sec_hue, hsv_info->hour_hue, clockwise);
            }
            // If Hour is OFF, we keep sec_hue (set at top of block)
        }
    } 
    else {
        // [Sec = OFF]
        if (min_on) {
            // [Sec = OFF, Min = ON]
            *h = hsv_info->min_hue; // Start with Min hue

            if (hour_on) {
                // [Sec = OFF, Min = ON, Hour = ON] -> Blend Min+Hour
                // Original: if(clockwise) { if(hour > min) ... }
                // Argument 1 is HOUR, Argument 2 is MIN
                *h = blend(hsv_info->hour_hue, hsv_info->min_hue, clockwise);
            }
            // If Hour is OFF, we keep min_hue
        } 
        else {
            // [Sec = OFF, Min = OFF]
            if (hour_on) {
                // [Sec = OFF, Min = OFF, Hour = ON] -> Blue
                *h = hsv_info->hour_hue;
            } else {
                // [ALL OFF]
                *h = 0;
                *s = 0;
                *v = 0;
            }
        }
    }
}

static void update_strip(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    struct hsv_struct hsv_info;
    hsv_info.bcd = true;
    hsv_info.reverse = true;
    hsv_info.sec_hue = 0;
    hsv_info.min_hue = 120;
    hsv_info.hour_hue = 240;
    uint8_t min_indx = get_min_hue_index(hsv_info.hues);
    hsv_info.clockwise = 
        hsv_info.hues[(min_indx+1)%3] < hsv_info.hues[(min_indx+2)%3] ? 
        true : false;
    ESP_LOGD(TAG,"min_indx = %d, clockwise = %d",min_indx, hsv_info.clockwise);

    for (int i = 0; i < N_LEDS; ++i) {
        uint32_t red = 0, green = 0, blue = 0;
        uint16_t hue = 0;
        uint8_t saturation = 100, value = BRIGHTNESS;
        hsv_info.led_no = i;
        make_hsv(&timeinfo, &hsv_info, &hue, &saturation, &value);
        led_strip_hsv2rgb(hue, saturation, value, &red, &green, &blue);
        led_strip_set_pixel(strip_leds, i, red, green, blue);
    }
    led_strip_refresh(strip_leds);
}

void Blink_Task(void *arg)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    while (1) {
        update_strip();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
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
