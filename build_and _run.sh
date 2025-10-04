#!/bin/bash
set -e

# Đường dẫn vcpkg tương đối
VCPKG_DIR="./vcpkg"

# Kiểm tra vcpkg
if [ ! -d "$VCPKG_DIR" ]; then
  echo "[INFO] vcpkg chưa có, tiến hành cài đặt..."
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
  pushd "$VCPKG_DIR"
  ./bootstrap-vcpkg.sh
  popd
else
  echo "[INFO] vcpkg đã tồn tại."
fi

# Kiểm tra và cài package cần thiết
echo "[INFO] Cài đặt các package cần thiết..."
$VCPKG_DIR/vcpkg install curl:x64-windows
$VCPKG_DIR/vcpkg install nlohmann-json:x64-windows

# Biên dịch
echo "[INFO] Biên dịch chương trình..."
rm -f main.exe
/c/mingw64/bin/g++ -g main.cc esp_32_music.cc -o main.exe \
  -I$VCPKG_DIR/installed/x64-windows/include \
  -L$VCPKG_DIR/installed/x64-windows/lib \
  -lcurl -lwinmm

echo "[INFO] Chạy chương trình..."
./main.exe
