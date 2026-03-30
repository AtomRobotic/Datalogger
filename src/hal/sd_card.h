#ifndef SD_CARD_API_H
#define SD_CARD_API_H

/* Includes ------------------------------------------------------------------*/
#include <common.h>
#include "sd.h"
#include <SPI.h>
/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
bool sd_init(uint8_t csPin, SPIClass *spi);

/**
 * @brief Ghi nối tiếp dữ liệu (Dùng cho Backup Datalogger)
 * Ghi dữ liệu vào cuối file và thêm dòng mới
 */
bool sd_append(const char *path, const char *message);

/**
 * @brief Kiểm tra dung lượng còn lại (MB)
 */
float sd_get_free_space_mb();

/**
 * @brief Lấy tổng dung lượng thẻ nhớ (MB)
 */
float sd_get_total_space_mb();

/**
 * @brief Ghi đè dữ liệu (Dùng cho lưu cấu hình)
 */
bool sd_write(const char *path, const uint8_t *data, size_t len);

/**
 * @brief Đọc dữ liệu
 */
size_t sd_read(const char *path, uint8_t *data, size_t len);

/**
 * @brief Cắt bỏ phần dữ liệu đã gửi thành công trong file để tránh gửi trùng
 * Lưu ý: Hàm này sẽ tạo file tạm thời và ghi đè lên file gốc, đảm bảo không để lại rác
 */
bool sd_truncate_file(const char *filepath, size_t offset);

#endif // SD_CARD_API_H