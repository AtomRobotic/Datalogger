// /* Includes ------------------------------------------------------------------*/
// #include "sd_card.h"
// #include <FS.h>
// #include <SD.h>

// /* Variables -----------------------------------------------------------------*/
// static bool sdReady = false;
// static const char *TAG_SD = "SD_CARD";

// // Mutex để bảo vệ quá trình đọc/ghi SD Card tránh xung đột giữa các Task
// static SemaphoreHandle_t s_sd_mutex = NULL;

// /* Functions -----------------------------------------------------------------*/

// /**
//  * @brief Khởi tạo SD card với custom SPI và Mutex bảo vệ
//  */
// bool sd_init(uint8_t csPin, SPIClass *spi)
// {
//     // Tạo Mutex nếu chưa có
//     if (s_sd_mutex == NULL)
//     {
//         s_sd_mutex = xSemaphoreCreateMutex();
//         if (s_sd_mutex == NULL)
//         {
//             APP_LOGE(TAG_SD, "Failed to create SD Mutex!");
//             return false;
//         }
//     }

//     if (!SD.begin(csPin, *spi, 4000000U)) // Giới hạn tốc độ SPI ở 4MHz để ổn định hơn với module SD
//     {
//         APP_LOGE(TAG_SD, "SD init failed! Check wiring or card insertion.");
//         sdReady = false;
//         return false;
//     }

//     uint8_t cardType = SD.cardType();
//     if (cardType == CARD_NONE)
//     {
//         APP_LOGE(TAG_SD, "No SD card attached.");
//         sdReady = false;
//         return false;
//     }

//     uint64_t cardSize = SD.cardSize() / (1024 * 1024);
//     APP_LOGI(TAG_SD, "SD Card Initialized. Type: %d, Size: %llu MB", cardType, cardSize);

//     sdReady = true;
//     return true;
// }

// /**
//  * @brief Ghi nối tiếp dữ liệu (Dùng cho Backup Datalogger) - THREAD SAFE
//  */
// bool sd_append(const char *path, const char *message)
// {
//     if (!sdReady || s_sd_mutex == NULL)
//         return false;

//     bool result = false;

//     // Lấy chìa khóa Mutex, chờ tối đa 1 giây. Nếu có Task khác đang dùng thẻ nhớ, Task này sẽ chờ.
//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
//     {
//         File file = SD.open(path, FILE_APPEND);
//         if (!file)
//         {
//             APP_LOGE(TAG_SD, "Failed to open file for appending: %s", path);
//         }
//         else
//         {
//             if (file.println(message))
//             {
//                 result = true;
//             }
//             else
//             {
//                 APP_LOGE(TAG_SD, "Append failed on: %s", path);
//             }
//             file.close();
//         }
//         // Xong việc phải trả lại Mutex ngay lập tức
//         xSemaphoreGive(s_sd_mutex);
//     }
//     else
//     {
//         APP_LOGW(TAG_SD, "SD Card is busy. Append timeout!");
//     }

//     return result;
// }

// /**
//  * @brief Cắt bỏ phần đầu của file (Tính năng dọn dẹp sau khi đẩy Backup lên MQTT)
//  * Tránh nổ RAM và Watchdog bằng cách copy từng block nhỏ (Chunking)
//  */
// bool sd_truncate_file(const char *filepath, size_t offset)
// {
//     if (!sdReady || s_sd_mutex == NULL)
//         return false;

//     bool result = false;

//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(2000)) == pdTRUE)
//     {
//         File original = SD.open(filepath, FILE_READ);
//         if (!original)
//         {
//             xSemaphoreGive(s_sd_mutex);
//             return false;
//         }

//         // Nếu offset vượt quá hoặc bằng kích thước file, nghĩa là đã xử lý hết -> Xóa luôn
//         if (offset >= original.size())
//         {
//             original.close();
//             SD.remove(filepath);
//             xSemaphoreGive(s_sd_mutex);
//             return true;
//         }

//         const char *temp_path = "/sys_temp_cut.txt";
//         File temp = SD.open(temp_path, FILE_WRITE);

//         if (!temp)
//         {
//             APP_LOGE(TAG_SD, "Failed to create temp file for truncation.");
//             original.close();
//             xSemaphoreGive(s_sd_mutex);
//             return false;
//         }

//         // Dời con trỏ tới vị trí cần giữ lại
//         original.seek(offset);

//         // Chunk copy: Đọc ghi từng block 512 byte để thẻ nhớ hoạt động tối ưu nhất
//         uint8_t buf[512];
//         while (original.available())
//         {
//             size_t bytesRead = original.read(buf, sizeof(buf));
//             temp.write(buf, bytesRead);

//             // Nhường CPU cho các Task khác sống (đặc biệt quan trọng với file lớn)
//             vTaskDelay(pdMS_TO_TICKS(2));
//         }

//         original.close();
//         temp.close();

//         // Ghi đè file mới lên file cũ
//         SD.remove(filepath);
//         result = SD.rename(temp_path, filepath);

//         if (!result)
//         {
//             APP_LOGE(TAG_SD, "Rename failed during truncation!");
//         }

//         xSemaphoreGive(s_sd_mutex);
//     }

//     return result;
// }

// /**
//  * @brief Kiểm tra dung lượng còn lại (MB)
//  */
// float sd_get_free_space_mb()
// {
//     if (!sdReady)
//         return 0;
//     // Bọc Mutex cho an toàn tuyệt đối
//     float space = 0;
//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
//     {
//         uint64_t total = SD.totalBytes();
//         uint64_t used = SD.usedBytes();
//         space = (float)(total - used) / (1024 * 1024);
//         xSemaphoreGive(s_sd_mutex);
//     }
//     return space;
// }

// /**
//  * @brief Lấy tổng dung lượng thẻ nhớ (MB)
//  */
// float sd_get_total_space_mb()
// {
//     if (!sdReady)
//         return 0;
//     float space = 0;
//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
//     {
//         space = (float)SD.totalBytes() / (1024 * 1024);
//         xSemaphoreGive(s_sd_mutex);
//     }
//     return space;
// }

// /**
//  * @brief Ghi đè toàn bộ file dữ liệu (Dùng cho lưu file cấu hình)
//  */
// bool sd_write(const char *path, const uint8_t *data, size_t len)
// {
//     if (!sdReady || s_sd_mutex == NULL)
//         return false;

//     bool result = false;
//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
//     {
//         File file = SD.open(path, FILE_WRITE);
//         if (!file)
//         {
//             APP_LOGE(TAG_SD, "Open for write failed: %s", path);
//         }
//         else
//         {
//             size_t written = file.write(data, len);
//             result = (written == len);
//             file.close();
//         }
//         xSemaphoreGive(s_sd_mutex);
//     }
//     return result;
// }

// /**
//  * @brief Đọc dữ liệu từ file
//  */
// size_t sd_read(const char *path, uint8_t *data, size_t len)
// {
//     if (!sdReady || s_sd_mutex == NULL)
//         return 0;

//     size_t total = 0;
//     if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
//     {
//         File file = SD.open(path, FILE_READ);
//         if (!file)
//         {
//             APP_LOGE(TAG_SD, "Open for read failed: %s", path);
//         }
//         else
//         {
//             // Tối ưu hàm đọc: Đọc 1 block thay vì vòng lặp từng byte
//             total = file.read(data, len);
//             file.close();
//         }
//         xSemaphoreGive(s_sd_mutex);
//     }
//     return total;
// }