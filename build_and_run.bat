@echo off
REM Thiết lập UTF-8 để hiển thị tiếng Việt
chcp 65001 >nul

REM Xóa file cũ nếu có
if exist main.exe del main.exe

REM Kiểm tra vcpkg
if not exist vcpkg (
    echo [INFO] vcpkg chưa có, tiến hành cài đặt...
    git clone https://github.com/microsoft/vcpkg.git vcpkg
    cd vcpkg
    bootstrap-vcpkg.bat
    cd ..
) else (
    echo [INFO] vcpkg đã tồn tại.
)

REM Cài đặt package cần thiết
vcpkg\vcpkg.exe install curl:x64-windows
vcpkg\vcpkg.exe install nlohmann-json:x64-windows

REM Biên dịch
echo [INFO] Biên dịch chương trình...
"C:\msys64\mingw64\bin\g++.exe" -g main.cc esp_32_music.cc -o main.exe ^
 -I vcpkg\installed\x64-windows\include ^
 -L vcpkg\installed\x64-windows\lib ^
 -lcurl -lwinmm

REM Chạy chương trình
echo [INFO] Chạy chương trình...
main.exe
pause
