#include "ntp_time.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

// Múi giờ Việt Nam (GMT+7)
static const char* ntp_server           = "pool.ntp.org";
static const long  gmt_offset_sec      = 7 * 3600;  // 7 giờ
static const int   daylight_offset_sec = 0;         // Không có DST
static char        time_buffer[32];                 // Lưu chuỗi thời gian đã định dạng
static bool        is_time_synced      = false;
static time_t      last_epoch_time     = 0;         // Lưu thời gian epoch để tính lại khi cần

void ntp_time_init() {
    configTime(gmt_offset_sec, daylight_offset_sec, "time1.google.com", "time2.google.com", "time.nist.gov");
    is_time_synced = false;
    memset(time_buffer, 0, sizeof(time_buffer));
    last_epoch_time = 0;
}

bool ntp_time_is_synced() {
    return is_time_synced;
}

bool ntp_update_time() {
    struct tm timeinfo;

    // Cập nhật thời gian nếu đã có internet & NTP sẵn sàng
    if (!getLocalTime(&timeinfo)) {
        snprintf(time_buffer, sizeof(time_buffer), "Time not synced");
        is_time_synced = false;
        return false;
    }

    // Nếu lấy được thời gian, ghi lại mốc epoch
    time(&last_epoch_time);
    is_time_synced = true;

    // Định dạng lại chuỗi thời gian lưu trữ sẵn
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    return true;
}

char* ntp_time_get_buffer(void) {
    // Nếu chưa từng đồng bộ thì trả mặc định
    if (!is_time_synced || last_epoch_time == 0) {
        snprintf(time_buffer, sizeof(time_buffer), "Time not synced");
        return time_buffer;
    }

    // Tính thời gian hiện tại dựa trên last_epoch_time + uptime
    time_t now = time(NULL); // epoch hiện tại
    struct tm* ptm = localtime(&now);

    if (!ptm) {
        snprintf(time_buffer, sizeof(time_buffer), "Time error");
        return time_buffer;
    }

    // Tạo chuỗi mới từ thời gian hiện tại
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", ptm);
    return time_buffer;
}
