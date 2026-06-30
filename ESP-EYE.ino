#include <Deteccion_Enfermedades_de_la_Vid_inferencing.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_camera.h"

#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"

typedef struct struct_message {
    uint8_t command;
    uint8_t brightness;
} struct_message;
struct_message receivedData;

typedef struct struct_response {
    char disease[32];
    float confidence;
} struct_response;
struct_response responseData;

uint8_t displayMAC[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 Eye - IA v2.0");
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return;
    }
    
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, displayMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    
    Serial.println("Listo para recibir comandos");
}

void loop() {
    delay(100);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    
    if (receivedData.command == 1) {
        processImage();
    }
}

void processImage() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    
    uint8_t *rgb_buf = (uint8_t*)malloc(320 * 240 * 3);
    if (!rgb_buf) {
        esp_camera_fb_return(fb);
        return;
    }
    fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb_buf);
    esp_camera_fb_return(fb);
    
    float features[EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT];
    int idx = 0;
    for (int y = 0; y < EI_CLASSIFIER_INPUT_HEIGHT; y++) {
        for (int x = 0; x < EI_CLASSIFIER_INPUT_WIDTH; x++) {
            int pixel_idx = (y * EI_CLASSIFIER_INPUT_WIDTH + x) * 3;
            float r = rgb_buf[pixel_idx] / 255.0f;
            float g = rgb_buf[pixel_idx + 1] / 255.0f;
            float b = rgb_buf[pixel_idx + 2] / 255.0f;
            features[idx++] = (r + g + b) / 3.0f;
        }
    }
    free(rgb_buf);
    
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = [&features](size_t offset, size_t length, float *out_ptr) {
        for (size_t i = 0; i < length; i++) {
            out_ptr[i] = features[offset + i];
        }
        return 0;
    };
    
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    
    if (err == EI_IMPULSE_OK) {
        float max_conf = 0;
        int max_idx = 0;
        for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (result.classification[i].value > max_conf) {
                max_conf = result.classification[i].value;
                max_idx = i;
            }
        }
        strcpy(responseData.disease, ei_classifier_inferencing_categories[max_idx]);
        responseData.confidence = max_conf;
    } else {
        strcpy(responseData.disease, "Error");
        responseData.confidence = 0;
    }
    
    esp_now_send(displayMAC, (uint8_t*)&responseData, sizeof(responseData));
}