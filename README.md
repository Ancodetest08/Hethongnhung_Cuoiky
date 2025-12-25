# **Smart Fan Control System**

## Thông tin dự án

**Môn học:** Hệ thống nhúng  
**Loại:** Đồ án cuối kỳ  
**Giảng viên hướng dẫn:** ThS. Hồ Viết Việt  
**Nhóm:** A2D  
**Thành viên:**
- Nguyễn Ngọc Đức
- Trương Văn An
- Lê Thị Ánh Trinh

---

## 1. Mô tả tổng quan

Hệ thống điều khiển quạt thông minh được xây dựng trên nền tảng **ESP32-C3** với **FreeRTOS**, tích hợp đầy đủ các tính năng IoT hiện đại:
- **Cảm biến DHT22:** Giám sát nhiệt độ và độ ẩm real-time
- **Chế độ Auto/Manual:** Tự động điều chỉnh hoặc điều khiển thủ công
- **Web Interface:** Điều khiển từ xa qua WiFi
- **Timer:** Hẹn giờ tắt tự động
- **LED RGB:** Hiển thị trạng thái bằng màu sắc và hiệu ứng
- **Nút bấm:** Điều khiển nhanh không cần Web

---

## 2. Tính năng chính

### Hai chế độ hoạt động
#### **1. Chế độ AUTO (Tự động)**
- Tự động điều chỉnh tốc độ quạt dựa trên nhiệt độ môi trường
- LED RGB hiệu ứng **breathing** (thở nhẹ) để nhận biết chế độ Auto
- 4 mức tốc độ tự động:
  - **OFF (0):** Nhiệt độ ≤ 27°C → LED xám nhấp nháy
  - **LOW (1):** 27°C < Nhiệt độ ≤ 29°C → LED xanh lá
  - **MEDIUM (2):** 29°C < Nhiệt độ ≤ 32°C → LED vàng
  - **HIGH (3):** Nhiệt độ > 32°C → LED đỏ

#### **2. Chế độ MANUAL (Thủ công)**
- Điều khiển tốc độ quạt bằng nút bấm hoặc Web interface
- LED RGB hiển thị màu cố định (không nhấp nháy)
- 4 mức tốc độ:
  - **0:** OFF → LED tắt
  - **1:** LOW (3000 duty) → LED xanh
  - **2:** MEDIUM (5000 duty) → LED vàng
  - **3:** HIGH (8000 duty) → LED đỏ

### Giám sát môi trường
- **Cảm biến:** DHT22 (nhiệt độ và độ ẩm)
- **Tần suất đọc:** Mỗi 3 giây
- **Hiển thị:** Real-time trên Web interface với độ chính xác 0.1°C

### Web Interface (Responsive)
- Xem nhiệt độ và độ ẩm real-time
- Chuyển đổi chế độ Auto ↔ Manual (toggle switch)
- Điều khiển tốc độ quạt (4 nút: OFF, LOW, MID, HIGH)
- Cài đặt ngưỡng nhiệt độ cho chế độ Auto (3 mức)
- Hẹn giờ tắt quạt (Timer countdown)
- Giao diện đẹp, hiện đại, tương thích mobile

### Timer tự động tắt
- Hẹn giờ tắt quạt theo phút
- Hiển thị thời gian còn lại trên Web
- Tự động tắt quạt khi hết giờ
- Có thể hủy bất kỳ lúc nào

### Điều khiển nút bấm
- **Nhấn ngắn:** Thay đổi tốc độ quạt (trong chế độ Manual: 0→1→2→3→0)
- **Nhấn giữ 1.5 giây:** Chuyển đổi Auto ↔ Manual

---

## 3. Các tính năng FreeRTOS đầy đủ

Project này triển khai **đầy đủ 6 tính năng FreeRTOS** theo yêu cầu môn học:

### **1. Tasks (5 tasks)**
| Task             | Priority | Stack | Chức năng                                              |
|------------------|----------|-------|--------------------------------------------------------|
| **vTaskButton**  | 6        | 2048  | Đọc nút bấm, xử lý debouncing, phát hiện nhấn ngắn/giữ |
| **vTaskControl** | 5        | 4096  | Bộ não trung tâm: xử lý Queue, điều khiển logic        |
| **vTaskLed**     | 5        | 2048  | Điều khiển LED RGB, hiệu ứng fade breathing            |
| **vTaskSensor**  | 4        | 2048  | Đọc DHT22 mỗi 3 giây, gửi data vào Queue               |
| **vTaskMonitor** | 4        | 2048  | Giám sát hệ thống, đợi WiFi kết nối                    |

### **2. Queue (xQueueCmd)**
Truyền lệnh giữa các task với 8 loại command:
```c
typedef enum {
    CMD_TOGGLE_MODE,      // Chuyển Auto/Manual (từ Button)
    CMD_CHANGE_SPEED,     // Thay đổi tốc độ (từ Button)
    CMD_UPDATE_TEMP,      // Cập nhật nhiệt độ/độ ẩm (từ Sensor)
    CMD_WEB_SET_SPEED,    // Đặt tốc độ (từ Web)
    CMD_WEB_SET_MODE,     // Đặt chế độ (từ Web)
    CMD_WEB_SET_THRESH,   // Đặt ngưỡng nhiệt độ (từ Web)
    CMD_WEB_SET_TIMER,    // Đặt timer (từ Web)
    CMD_TIMER_EXPIRED     // Timer hết giờ (từ Software Timer)
} command_type_t;
```

### **3. Mutex (xMutexState)**
Bảo vệ biến toàn cục `g_state` chứa trạng thái hệ thống:
```c
typedef struct {
    bool is_auto;           // Chế độ Auto/Manual
    int speed_level;        // Tốc độ hiện tại (0-3)
    float current_temp;     // Nhiệt độ hiện tại
    float current_hum;      // Độ ẩm hiện tại
    float thresh_1, thresh_2, thresh_3;  // Ngưỡng nhiệt độ
    int timer_minutes;      // Thời gian timer còn lại
    bool timer_active;      // Timer có đang chạy không
} system_state_t;
```

### **4. Task Notification (xTaskLedHandle)**
Gửi tín hiệu đến LED Task để reset hiệu ứng khi thay đổi chế độ/tốc độ:
```c
xTaskNotifyGive(xTaskLedHandle);  // Gửi notification
ulTaskNotifyTake(pdTRUE, 0);      // Nhận notification trong LED Task
```

### **5. Event Group (xEventGroupSys)**
Đồng bộ khởi động hệ thống:
- **WIFI_CONNECTED_BIT:** Đánh dấu WiFi đã kết nối thành công
- Monitor Task chờ bit này trước khi hoạt động
```c
xEventGroupWaitBits(xEventGroupSys, WIFI_CONNECTED_BIT, ...);
```

### **6. Software Timer (xTimerShutdown)**
Timer đếm ngược tự động tắt quạt:
- Khi hết giờ → gọi callback → gửi `CMD_TIMER_EXPIRED` vào Queue
- Control Task xử lý: tắt quạt, chuyển về Manual mode
```c
void vTimerCallback(TimerHandle_t xTimer) {
    app_command_t cmd = { .type = CMD_TIMER_EXPIRED };
    xQueueSend(xQueueCmd, &cmd, 0);
}
```

---

## 4. Phần cứng

### Linh kiện cần thiết
- **Vi điều khiển:** ESP32-C3 (4MB Flash)
- **Cảm biến:** DHT22 (nhiệt độ -40~80°C, độ ẩm 0-100%)
- **Actuator:** Quạt DC 12V với driver (L298N/IRF520/transistor)
- **LED:** LED RGB WS2812 (1 LED)
- **Nút bấm:** 1 nút bấm tactile switch
- **Nguồn:** 5V cho ESP32-C3, 12V cho quạt

### Sơ đồ kết nối GPIO
| Thành phần | GPIO Pin | Chức năng              |
|------------|----------|------------------------|
| DHT22 Data | GPIO 5   | Đọc nhiệt độ/độ ẩm     |
| Quạt PWM   | GPIO 4   | Điều khiển tốc độ quạt |
| LED RGB    | GPIO 8   | WS2812 LED strip       |
| Nút bấm    | GPIO 9   | Input với pull-up      |

### Cấu hình PWM cho quạt
```c
#define FAN_SPEED_LOW   3000   // Duty cycle 36.6% (3000/8191)
#define FAN_SPEED_MID   5000   // Duty cycle 61.0%
#define FAN_SPEED_HIGH  8000   // Duty cycle 97.7%
```

---

## 5. Cấu trúc thư mục project

```
Hethongnhung_CuoiKy/
├── main/
│   ├── main.c                     # Entry point, 5 tasks
│   ├── CMakeLists.txt
│   └── idf_component.yml          # Dependencies (led_strip)
│
├── components/
│   ├── my_common/                 # Module chung hệ thống
│   │   ├── include/
│   │   │   ├── app_config.h       # Cấu hình GPIO, PWM
│   │   │   └── app_system.h       # Khai báo Queue, Mutex, State
│   │   ├── app_system.c           # Khởi tạo FreeRTOS objects
│   │   └── CMakeLists.txt
│   │
│   ├── my_drivers/                # Module điều khiển phần cứng
│   │   ├── include/
│   │   │   ├── app_driver.h       # Khai báo LED, Fan
│   │   │   └── dht.h              # Khai báo DHT22 driver
│   │   ├── app_driver.c           # LED RGB, Fan PWM
│   │   ├── dht.c                  # DHT22 protocol
│   │   └── CMakeLists.txt
│   │
│   └── my_network/                # Module mạng
│       ├── include/
│       │   ├── app_wifi.h         # Khai báo WiFi
│       │   └── app_webserver.h    # Khai báo Web server
│       ├── app_wifi.c             # WiFi STA connection
│       ├── app_webserver.c        # HTTP server + HTML
│       └── CMakeLists.txt
│
├── CMakeLists.txt                 # Root CMake
├── sdkconfig.defaults             # Flash 4MB
└── README.md
```

---

## 6. Hướng dẫn cài đặt

### Yêu cầu hệ thống
- **ESP-IDF:** v4.4 trở lên (khuyến nghị v5.0+)
- **Python:** 3.7+
- **Phần cứng:** ESP32-C3 dev board
- **Cable:** USB type-C hoặc micro-USB

### Bước 1: Clone repository
```bash
git clone https://github.com/ancodetest08/Hethongnhung_CuoiKy.git
cd Hethongnhung_CuoiKy
```

### Bước 2: Cấu hình WiFi
Mở file `components/my_network/app_wifi.c` và sửa:
```c
#define WIFI_SSID      "Ten_WiFi_Cua_Ban"
#define WIFI_PASS      "Mat_Khau_WiFi"
```

### Bước 3: (Tùy chọn) Điều chỉnh GPIO
Nếu cần đổi GPIO, chỉnh sửa `components/my_common/include/app_config.h`:
```c
#define PIN_LED_RGB     8   // LED RGB WS2812
#define PIN_FAN_PWM     4   // Fan PWM
#define PIN_BUTTON      9   // Button
#define PIN_DHT         5   // DHT22
```

### Bước 4: Build project
```bash
idf.py build
```

### Bước 5: Flash vào ESP32-C3
```bash
idf.py flash
```

### Bước 6: Xem log và lấy IP
```bash
idf.py monitor
```

Trong log, tìm dòng:
```
I (xxxx) WIFI: Got IP: 192.168.1.XXX
```

### Bước 7: Truy cập Web Interface
Mở trình duyệt và truy cập:
```
http://192.168.1.XXX
```

---

## 7. Hướng dẫn sử dụng Web Interface

### Giao diện chính

#### **Panel 1: Sensor Display**
- Hiển thị nhiệt độ (°C) và độ ẩm (%) real-time
- Cập nhật mỗi giây

#### **Panel 2: Control Fan**
- **Toggle Switch:** Chuyển AUTO ↔ MANUAL
- **4 nút điều khiển:**
  - LOW → Tốc độ thấp
  - MID → Tốc độ trung bình
  - HIGH → Tốc độ cao
  - POWER OFF → Tắt quạt
- Nút đang active sẽ có màu xanh dương

#### **Panel 3: Auto Mode Settings**
- Cài đặt 3 ngưỡng nhiệt độ cho chế độ Auto
- **Level 1 (>):** Ngưỡng bật tốc độ LOW (mặc định 27°C)
- **Level 2 (>):** Ngưỡng bật tốc độ MEDIUM (mặc định 29°C)
- **Level 3 (>):** Ngưỡng bật tốc độ HIGH (mặc định 32°C)
- Nhấn **Save Config** để lưu

#### **Panel 4: Timer Countdown**
- Nhập số phút muốn hẹn giờ
- Nhấn **Start** để bắt đầu đếm ngược
- Hiển thị thời gian còn lại (ví dụ: "Off in 5 mins")
- Nhập 0 và Start để hủy timer

### API Endpoints

#### GET `/`
Trả về HTML page

#### GET `/status`
Lấy trạng thái hệ thống (JSON)
```json
{
  "t": 28.5,        // Nhiệt độ (°C)
  "h": 65.0,        // Độ ẩm (%)
  "spd": 2,         // Tốc độ hiện tại (0-3)
  "auto": 1,        // Chế độ Auto (1) hay Manual (0)
  "th1": 27.0,      // Ngưỡng level 1
  "th2": 29.0,      // Ngưỡng level 2
  "th3": 32.0,      // Ngưỡng level 3
  "tm": 0           // Timer còn lại (phút)
}
```

#### GET `/ctrl?cmd=mode`
Chuyển đổi chế độ Auto/Manual

#### GET `/ctrl?cmd=speed&val=X`
Đặt tốc độ quạt (X = 0, 1, 2, 3)

#### GET `/ctrl?cmd=thresh&t1=X&t2=Y&t3=Z`
Đặt ngưỡng nhiệt độ

#### GET `/ctrl?cmd=timer&val=X`
Đặt timer X phút (0 = hủy)

---

## 8. Sơ đồ luồng hoạt động

### Luồng chính của hệ thống
```
[app_main]
    │
    ├─> app_system_init()     → Tạo Queue, Mutex, Event Group, Timer
    ├─> app_driver_init()     → Khởi tạo GPIO, PWM, LED
    ├─> app_wifi_init()       → Kết nối WiFi
    ├─> app_webserver_init()  → Khởi động HTTP server
    │
    ├─> vTaskControl    (Priority 5)
    ├─> vTaskButton     (Priority 6) ← Highest
    ├─> vTaskSensor     (Priority 4)
    ├─> vTaskLed        (Priority 5)
    └─> vTaskMonitor    (Priority 4)
```

### Luồng xử lý chế độ Auto
```
[vTaskSensor]
    │
    └─> Đọc DHT22 mỗi 3 giây
        │
        └─> Gửi CMD_UPDATE_TEMP vào Queue
            │
            ▼
[vTaskControl]
    │
    └─> Nhận command từ Queue
        │
        └─> Kiểm tra if (is_auto == true)
            │
            ├─> Temp > thresh_3  → Speed = 3 (HIGH)
            ├─> Temp > thresh_2  → Speed = 2 (MID)
            ├─> Temp > thresh_1  → Speed = 1 (LOW)
            └─> Temp ≤ thresh_1  → Speed = 0 (OFF)
            │
            └─> fan_set_speed(speed) + xTaskNotifyGive(LED)
                │
                ▼
[vTaskLed]
    │
    └─> Nhận notification → Reset LED
        │
        └─> Hiển thị màu theo speed + breathing effect
```

### Luồng xử lý nút bấm
```
[vTaskButton]
    │
    ├─> Nhấn giữ 1.5s → CMD_TOGGLE_MODE → Queue
    │                      │
    │                      ▼
    │                [vTaskControl]
    │                      │
    │                      └─> Toggle is_auto
    │                          │
    │                          └─> Notify LED Task
    │
    └─> Nhấn ngắn → CMD_CHANGE_SPEED → Queue
                       │
                       ▼
                [vTaskControl]
                       │
                       └─> if (!is_auto)
                           │
                           └─> speed_level++
                               │
                               └─> fan_set_speed() + Notify LED
```

### Luồng timer tự động tắt
```
[Web Interface]
    │
    └─> User nhập X phút → POST /ctrl?cmd=timer&val=X
        │
        ▼
[HTTP Handler]
    │
    └─> Gửi CMD_WEB_SET_TIMER vào Queue
        │
        ▼
[vTaskControl]
    │
    └─> xTimerChangePeriod(xTimerShutdown, X phút)
        │
        └─> xTimerStart()
            │
            └─> Đếm ngược...
                │
                └─> Hết giờ → vTimerCallback()
                    │
                    └─> Gửi CMD_TIMER_EXPIRED vào Queue
                        │
                        ▼
                [vTaskControl]
                        │
                        └─> Tắt quạt
                            └─> is_auto = false
                                └─> speed_level = 0
```

---

## 9. LED RGB Indicator

| Chế độ     | Tốc độ   | Màu LED           | Hiệu ứng  | Mô tả                   |
|------------|----------|-------------------|-----------|-------------------------|
| **AUTO**   | OFF (0)  | Xám (50,50,50)    | Breathing | Nhiệt độ thấp, quạt tắt |
| **AUTO**   | LOW (1)  | Xanh lá (0,255,0) | Breathing | Nhiệt độ vừa phải       |
| **AUTO**   | MID (2)  | Vàng (255,255,0)  | Breathing | Nhiệt độ cao            |
| **AUTO**   | HIGH (3) | Đỏ (255,0,0)      | Breathing | Nhiệt độ rất cao        |
| **MANUAL** | OFF (0)  | Tắt (0,0,0)       | Solid     | Người dùng tắt          |
| **MANUAL** | LOW (1)  | Xanh lá           | Solid     | Người dùng chọn         |
| **MANUAL** | MID (2)  | Vàng              | Solid     | Người dùng chọn         |
| **MANUAL** | HIGH (3) | Đỏ                | Solid     | Người dùng chọn         |

**Breathing effect:** LED fade từ 5% → 100% → 5% với tốc độ 20ms/frame

---

## 10. Kiến thức áp dụng

### FreeRTOS Concepts
**Tasks:** Multi-tasking với 5 tasks, priority từ 4-6  
**Queue:** Giao tiếp giữa tasks bằng message queue  
**Mutex:** Bảo vệ shared resource (g_state)  
**Task Notification:** Đồng bộ nhẹ giữa Control và LED  
**Event Group:** Chờ sự kiện WiFi kết nối  
**Software Timer:** Đếm ngược tự động tắt quạt  

### ESP32-C3 Peripherals
**GPIO:** Input (button) với pull-up, Output cho driver  
**LEDC (PWM):** Điều khiển tốc độ quạt với 13-bit resolution  
**RMT:** Điều khiển LED RGB WS2812  
**WiFi:** Station mode, event-driven  
**HTTP Server:** Xử lý requests, JSON response  
**NVS:** Non-Volatile Storage (khởi tạo)  

### Protocols & Standards
**DHT22 Protocol:** Đọc cảm biến bằng 1-wire protocol  
**HTTP/REST API:** GET requests với query parameters  
**JSON:** Serialize/deserialize dữ liệu  
**HTML/CSS/JavaScript:** Frontend responsive  

### Design Patterns
**Event-Driven Architecture:** Command pattern với Queue  
**State Machine:** Auto/Manual mode switching  
**Producer-Consumer:** Sensor → Queue → Control  
**Observer Pattern:** Web polling `/status` mỗi giây  

---

## 11. Tính năng có thể mở rộng

### **Nâng cấp phần cứng**
- [ ] Thêm màn hình OLED hiển thị nhiệt độ local
- [ ] Thêm cảm biến chất lượng không khí (MQ-135, PMS5003)
- [ ] Thêm relay điều khiển bật/tắt nguồn quạt hoàn toàn
- [ ] Thêm cảm biến ánh sáng (BH1750) tự động bật đèn

### **Lưu trữ dữ liệu**
- [ ] Lưu settings vào NVS (giữ sau khi reset)
- [ ] Ghi log lịch sử nhiệt độ vào SD card
- [ ] Xuất CSV data từ Web interface

### **Kết nối IoT**
- [ ] Tích hợp MQTT để điều khiển từ xa
- [ ] Kết nối với Blynk/Home Assistant
- [ ] Google Assistant/Alexa voice control
- [ ] Telegram bot để nhận cảnh báo

### **Giao diện nâng cao**
- [ ] Native mobile app (Flutter/React Native)
- [ ] Chart hiển thị biểu đồ nhiệt độ theo thời gian
- [ ] Dark mode cho Web interface
- [ ] Multi-language support (Tiếng Việt/English)

### **AI & Automation**
- [ ] Machine Learning dự đoán nhiệt độ
- [ ] Học thói quen người dùng để tối ưu
- [ ] Tích hợp với lịch thời tiết online
- [ ] Scene automation (sáng/tối/ngủ)

### **Bảo mật**
- [ ] Thêm login authentication cho Web
- [ ] HTTPS thay vì HTTP
- [ ] OTA (Over-The-Air) firmware update
- [ ] Rate limiting cho API

---

## 12. Tài liệu tham khảo

### ESP-IDF & FreeRTOS
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [FreeRTOS Official Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

### Datasheets
- [DHT22 Temperature & Humidity Sensor](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf)
- [WS2812B LED Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

### Tutorials
- [ESP32 PWM (LEDC) Tutorial](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html)
- [ESP32 WiFi Programming](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html)
- [ESP HTTP Server](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html)

---

## 13. Ghi chú quan trọng

### Thread Safety
- Tất cả truy cập vào `g_state` đều được bảo vệ bởi `xMutexState`
- Queue được sử dụng để truyền message an toàn giữa các task
- Task Notification nhẹ hơn Queue, dùng cho đồng bộ đơn giản

### Performance
- LED Task chạy với delay 20ms → 50 FPS cho hiệu ứng mượt mà
- Sensor đọc mỗi 3 giây → không spam Queue
- Web polling mỗi 1 giây → cân bằng giữa real-time và băng thông

### Debouncing
- Button debouncing: 20ms delay + check lại trạng thái
- Phát hiện nhấn giữ: 1500ms threshold
- Tránh nhiễu và false positive

### Power Management
- Có thể thêm `vTaskDelay()` trong idle loop để tiết kiệm điện
- PWM duty 0 = quạt tắt hoàn toàn (không tiêu thụ điện)

---

## 14. Đánh giá điểm mạnh của project

### **Kiến trúc phần mềm tốt**
- Modular design: chia thành 3 components rõ ràng
- Separation of concerns: mỗi task làm 1 việc
- Event-driven: không có busy-waiting

### **FreeRTOS đầy đủ**
- Sử dụng đủ 6 tính năng: Tasks, Queue, Mutex, Notification, Event Group, Timer
- Priority hợp lý: Button cao nhất (responsive), Monitor thấp nhất
- Thread-safe với Mutex

### **User Experience tốt**
- Web interface đẹp, responsive, dễ dùng
- LED feedback trực quan (màu + breathing)
- Nút bấm đơn giản nhưng hiệu quả (ngắn/giữ)

### **Tính năng thực tế**
- Auto mode tiết kiệm điện
- Timer tiện lợi cho việc ngủ
- Cài đặt threshold linh hoạt

### **Code quality**
- Comment rõ ràng (tiếng Việt)
- Naming convention nhất quán
- Error handling cho DHT22

---

## 15. Lời cảm ơn

- **ThS. Hồ Viết Việt** - Giảng viên hướng dẫn tận tình
- **Espressif Systems** - Cung cấp ESP-IDF framework tuyệt vời
- **FreeRTOS Community** - Documentation và examples
- **Khoa Điện tử - Viễn thông, Đại học Bách Khoa Đà Nẵng**

---

**Made with ❤️ using ESP32-C3 & FreeRTOS**