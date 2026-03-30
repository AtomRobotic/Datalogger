// /* Includes ------------------------------------------------------------------*/
// #include "backup_data.h"

// /* Define --------------------------------------------------------------------*/
// #define BACKUP_DIR "/backup"
// #define MAX_BACKUP_FILE_SIZE (20 * 1024) // Tối đa 20KB mỗi file (chia nhỏ để dễ quản lý)
// #define CHECK_INTERVAL_MS 10000          // 10 giây kiểm tra thư mục backup một lần

// static const char *TAG = "BACKUP_MGR";
// static String current_backup_file = "";

// /* Private Function Prototypes -----------------------------------------------*/
// static void ensure_backup_dir();
// static String get_new_backup_filename();
// static void save_to_sd(const char *data);
// static void truncate_file(const char* filepath, size_t offset);

// /* Functions -----------------------------------------------------------------*/

// /**
//  * @brief Đảm bảo thư mục backup tồn tại
//  */
// static void ensure_backup_dir() {
//     if (!SD.exists(BACKUP_DIR)) {
//         SD.mkdir(BACKUP_DIR);
//         Serial.println("Backup directory created.");
//     }
//     Serial.println("Backup directory ready.");
// }

// /**
//  * @brief Đếm số lượng file và sinh tên file tuần tự (vd: /backup/bk_1.txt, /backup/bk_2.txt)
//  */
// static String get_new_backup_filename() {
//     ensure_backup_dir();
//     File dir = SD.open(BACKUP_DIR);
//     int max_id = 0;
    
//     if (dir) {
//         while (true) {
//             File entry = dir.openNextFile();
//             if (!entry) break;
            
//             if (!entry.isDirectory()) {
//                 String name = String(entry.name());
//                 // Lọc lấy tên file nếu thư viện trả về cả đường dẫn
//                 int slashIndex = name.lastIndexOf('/');
//                 if (slashIndex >= 0) name = name.substring(slashIndex + 1);
                
//                 if (name.startsWith("bk_") && name.endsWith(".txt")) {
//                     String idStr = name.substring(3, name.indexOf(".txt"));
//                     int id = idStr.toInt();
//                     if (id > max_id) max_id = id;
//                 }
//             }
//             entry.close();
//         }
//         dir.close();
//     }
//     return String(BACKUP_DIR) + "/bk_" + String(max_id + 1) + ".txt";
// }

// /**
//  * @brief Hàm lưu data xuống thẻ nhớ và tự động sang trang file mới nếu file quá đầy
//  */
// static void save_to_sd(const char *data) {
//     ensure_backup_dir();
//     if (current_backup_file == "") {
//         current_backup_file = get_new_backup_filename();
//     }

//     File file = SD.open(current_backup_file.c_str(), FILE_APPEND);
//     if (!file) { // Fallback tạo lại file nếu lỗi
//         current_backup_file = get_new_backup_filename();
//         file = SD.open(current_backup_file.c_str(), FILE_APPEND);
//     }

//     if (file) {
//         file.println(data);
//         size_t size = file.size();
//         file.close();
        
//         APP_LOGI(TAG, "Saved to %s. Size: %d bytes. Free SD: %.2f MB", 
//                  current_backup_file.c_str(), size, sd_get_free_space_mb());

//         // Nếu file vượt quá dung lượng cho phép, reset tên để lần lưu sau tạo file mới
//         if (size > MAX_BACKUP_FILE_SIZE) {
//             current_backup_file = ""; 
//         }
//     } else {
//         APP_LOGE(TAG, "Critical Error: Failed to open/create backup file!");
//     }
// }

// /**
//  * @brief Cắt bỏ phần dữ liệu đã gửi thành công trong file để tránh gửi trùng
//  */
// static void truncate_file(const char* filepath, size_t offset) {
//     File original = SD.open(filepath, FILE_READ);
//     if (!original) return;

//     if (offset >= original.size()) {
//         original.close();
//         SD.remove(filepath);
//         return;
//     }

//     String temp_path = String(BACKUP_DIR) + "/temp_cut.txt";
//     File temp = SD.open(temp_path.c_str(), FILE_WRITE);
//     if (!temp) {
//         original.close();
//         return;
//     }

//     original.seek(offset);
//     uint8_t buf[256];
//     while (original.available()) {
//         size_t bytesRead = original.read(buf, sizeof(buf));
//         temp.write(buf, bytesRead);
//         vTaskDelay(pdMS_TO_TICKS(2)); // Nhường CPU để tránh Watchdog Timer
//     }

//     original.close();
//     temp.close();

//     SD.remove(filepath);
//     SD.rename(temp_path.c_str(), filepath);
// }

// /**
//  * @brief Hàm điều hướng dữ liệu
//  */
// void backup_manager_handle_data(const char *data)
// {
//     EventBits_t bits = xEventGroupGetBits(_normal_mode_event_group);

//     // Chỉ đẩy trực tiếp vào Queue nếu đang có mạng VÀ Queue trống hơn 2 slot (ưu tiên Modbus)
//     if ((bits & MQTT_CONNECTED_BIT) && uxQueueSpacesAvailable(_mqtt_outgoing_queue) > 5)
//     {
//         // Cấp phát động trên HEAP để tránh tràn STACK

//         mqtt_message_t *msg = (mqtt_message_t *)malloc(sizeof(mqtt_message_t));
//         if (msg != NULL) {
//             strncpy(msg->topic, _mqtt_topic_pub, sizeof(msg->topic) - 1);
//             strncpy(msg->payload, data, sizeof(msg->payload) - 1);
//             msg->topic[sizeof(msg->topic) - 1] = '\0';
//             msg->payload[sizeof(msg->payload) - 1] = '\0';

//             // Hàm xQueueSend sẽ tự copy dữ liệu vào vùng nhớ của Queue
//             if (xQueueSend(_mqtt_outgoing_queue, &msg, 0) != pdPASS) {
//                 APP_LOGW(TAG, "MQTT Queue full abruptly, redirecting to SD Card.");
//                 save_to_sd(data);
//                 free(msg); // SỬA ĐIỂM 2: Gửi Queue thất bại thì mới free
//             }
//         } else {
//             APP_LOGE(TAG, "Malloc failed! Low RAM, saving to SD.");
//             save_to_sd(data);
//         }
//     }
//     else
//     {
//         APP_LOGW(TAG, "System Offline or Queue Busy. Saving to SD...");
//         save_to_sd(data);
//     }
// }

// /**
//  * @brief Task ngầm xử lý đồng bộ dữ liệu file Backup cũ
//  */
// void task_backup_recovery(void *pvParameters)
// {
//     APP_LOGI(TAG, "Recovery Mode is started.");
//     ensure_backup_dir();

//     for (;;)
//     {
//         // 1. Đợi kết nối
//         xEventGroupWaitBits(_normal_mode_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

//         // 2. Mở thư mục backup
//         File dir = SD.open(BACKUP_DIR);
//         if (!dir) {
//             vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
//             continue;
//         }

//         bool connection_lost = false;

//         // 3. Quét từng file trong thư mục
//         while (true) {
//             File entry = dir.openNextFile();
//             if (!entry) break; // Hết file để đọc

//             if (entry.isDirectory()) {
//                 entry.close();
//                 continue;
//             }

//             String filename = String(BACKUP_DIR) + "/" + String(entry.name());
//             int slashIndex = filename.lastIndexOf('/');
//             String pureName = filename.substring(slashIndex + 1);

//             // Bỏ qua các file tạm
//             if (pureName == "temp_cut.txt") {
//                 entry.close();
//                 continue;
//             }

//             // Nếu file đang được đồng bộ chính là file đang ghi dở, giải phóng cờ để tạo file ghi mới
//             if (filename == current_backup_file) {
//                 current_backup_file = ""; 
//             }

//             APP_LOGI(TAG, "Syncing backup file: %s", filename.c_str());
            
//             bool file_fully_processed = true;
//             size_t processed_bytes = 0;

//             // 4. Đọc từng dòng của File
//             while (entry.available()) {
//                 vTaskDelay(pdMS_TO_TICKS(10)); // Nhường CPU

//                 EventBits_t bits = xEventGroupGetBits(_normal_mode_event_group);
//                 if (!(bits & MQTT_CONNECTED_BIT)) {
//                     APP_LOGW(TAG, "Connection lost during recovery. Pausing...");
//                     connection_lost = true;
//                     file_fully_processed = false;
//                     break;
//                 }

//                 // CHỜ QUEUE TRỐNG: Luôn ưu tiên chừa ít nhất 2 slot cho Modbus realtime
//                 while (uxQueueSpacesAvailable(_mqtt_outgoing_queue) <= 5) {
//                     vTaskDelay(pdMS_TO_TICKS(100)); // Sleep để Modbus hoạt động
//                     bits = xEventGroupGetBits(_normal_mode_event_group);
//                     if (!(bits & MQTT_CONNECTED_BIT)) break;
//                 }
                
//                 if (!(bits & MQTT_CONNECTED_BIT)) {
//                     connection_lost = true;
//                     file_fully_processed = false;
//                     break;
//                 }

//                 // Đọc dòng dữ liệu
//                 String line = entry.readStringUntil('\n');
//                 line.trim();

//                 if (line.length() > 0) {
//                     // Xin cấp phát bộ nhớ động (Tránh làm nổ RAM Stack của Task)
//                     mqtt_message_t *msg = (mqtt_message_t *)malloc(sizeof(mqtt_message_t));
                    
//                     if (msg != NULL) {
//                         strncpy(msg->topic, _mqtt_topic_pub, sizeof(msg->topic) - 1);
//                         strncpy(msg->payload, line.c_str(), sizeof(msg->payload) - 1);
//                         msg->topic[sizeof(msg->topic) - 1] = '\0';
//                         msg->payload[sizeof(msg->payload) - 1] = '\0';

//                         if (xQueueSend(_mqtt_outgoing_queue, &msg, pdMS_TO_TICKS(500)) == pdPASS) {
//                             processed_bytes = entry.position(); 
//                             vTaskDelay(pdMS_TO_TICKS(100)); 
//                             // LƯU Ý: KHÔNG ĐƯỢC FREE(MSG) Ở ĐÂY NỮA. Task MQTT sẽ lo việc đó!
//                         } else {
//                             APP_LOGW(TAG, "Queue congested, backing off.");
//                             entry.seek(processed_bytes); 
//                             file_fully_processed = false;
//                             free(msg); // SỬA ĐIỂM 2: Chỉ free nếu nhét vào Queue THẤT BẠI
//                             break; 
//                         }
//                     } else {
//                         APP_LOGE(TAG, "RAM exhausted! Pausing sync.");
//                         vTaskDelay(pdMS_TO_TICKS(1000));
//                         entry.seek(processed_bytes);
//                         file_fully_processed = false;
//                         break;
//                     }
//                 }
//             }

//             entry.close();

//             // 5. Xử lý file sau khi đọc
//             if (file_fully_processed) {
//                 // Xóa file nguyên vẹn cực kỳ nhanh, không để lại rác
//                 SD.remove(filename.c_str());
//                 APP_LOGI(TAG, "Completed & deleted file: %s", filename.c_str());
//             } else if (processed_bytes > 0 && !connection_lost) {
//                 // Cắt bỏ phần đầu đã gửi thành công để file nhẹ bớt
//                 APP_LOGI(TAG, "Truncating sent data from %s", filename.c_str());
//                 // truncate_file(filename.c_str(), processed_bytes);
//                 sd_truncate_file(filename.c_str(), processed_bytes); // Gọi trực tiếp từ Driver SD Card
//             }

//             if (connection_lost) break; // Thoát vòng lặp Folder để chờ mạng
//         }
        
//         dir.close();
//         vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
//     }
// }

// /**
//  * @brief Khởi tạo Task Backup Manager
//  */
// void init_backup_manager(void)
// {
//     SPI.begin(SCK, MISO, MOSI, SD_CS_PIN);
//     if (sd_init(SD_CS_PIN, &SPI))
//     {
//         APP_LOGI("MAIN", "SD Card OK. Backup feature is enabled.");
//         ensure_backup_dir();
//     }
//     else
//     {
//         APP_LOGE("MAIN", "SD Card failed. Data will be lost if offline!");
//     }
//     vTaskDelay(100);
    
//     // Vì không chứa Struct bự trên Stack nữa, ta có thể hạ size Stack xuống để nhường RAM
//     xTaskCreatePinnedToCore(
//         task_backup_recovery,
//         "BACKUP_RECOVERY",
//         1024 * 5, 
//         NULL,
//         5, // Độ ưu tiên thấp
//         NULL,
//         1);
// }