# # PlatformIO build system function - not a standard Python import
# Import("env")  # type: ignore[func-defined]
# import os
# import sys

# def sign_bin(source, target, env):
#     # 1. Lấy các đường dẫn cần thiết
#     platform = env.PioPlatform()
#     esptoolpy_dir = platform.get_package_dir("tool-esptoolpy")
#     espsecure_path = os.path.join(esptoolpy_dir, "espsecure.py")
    
#     # Đường dẫn file khóa của bạn
#     key_path = r"C:\Users\BachBuiSpr\Documents\PlatformIO\Projects\Datalogger_branch\Data_Logger_Inverter-main\scripts\secure_boot_signing_key.pem"
#     bin_file = str(target[0])
#     signed_temp = bin_file + ".signed"  # TẠO BIẾN FILE TẠM Ở ĐÂY
#     python_exe = sys.executable

#     print(f"\n--- Dang thuc hien ky firmware (Secure Boot V2) ---")

#     # 2. Xây dựng lệnh dưới dạng LIST để tránh lỗi tham số tham lam
#     cmd = [
#         python_exe, 
#         espsecure_path, 
#         "sign_data", 
#         "--version", "2", 
#         "--keyfile", key_path, 
#         "--output", signed_temp,  # CHỈ ĐỊNH ĐẦU RA LÀ FILE TẠM
#         bin_file                  # File nguồn cần ký
#     ]
    
#     # Chạy lệnh
#     result = env.Execute(" ".join([f'"{arg}"' for arg in cmd]))
    
#     if result == 0:
#         # GHI ĐÈ FILE: Xóa file bin gốc và đổi tên file tạm thành bin gốc
#         if os.path.exists(bin_file):
#             os.remove(bin_file)
#         os.rename(signed_temp, bin_file)

#         print(f"\n[SUCCESS] Firmware da duoc ky tai: {bin_file}")
        
#         # 3. Buớc kiểm tra (Verify) tự động sau khi ký
#         verify_cmd = [python_exe, espsecure_path, "verify_signature", "--version", "2", "--keyfile", key_path, bin_file]
#         env.Execute(" ".join([f'"{arg}"' for arg in verify_cmd]))
#     else:
#         print("\n[ERROR] Ky firmware that bai!")
#         # Dọn dẹp file tạm nếu quá trình ký thất bại
#         if os.path.exists(signed_temp):
#             os.remove(signed_temp)
#         env.Exit(1)

# env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", sign_bin)  # type: ignore[attr-defined]