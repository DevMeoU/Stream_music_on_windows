#include <iostream>
#include "esp_32_music.h"

int main() {
    MusicPlayer player;

    std::string ten_bai_hat;
    std::cout << "Nhập tên bài hát cần tìm: ";
    std::getline(std::cin, ten_bai_hat);

    // Tìm với gợi ý (chỉ lấy 1 kết quả như log mẫu)
    auto suggestions = player.search_with_suggestions(ten_bai_hat);
    if (suggestions.empty()) {
        std::cout << "Không tìm thấy bài hát nào phù hợp.\n";
        return 0;
    }

    // Hiển thị kết quả
    const auto& selected = suggestions[0];
    std::cout << "\nĐã tìm thấy bài hát:\n";
    std::cout << "Tiêu đề: " << selected.title << "\n";
    std::cout << "Nghệ sĩ: " << selected.artist << "\n";
    std::cout << "ID: " << selected.id << "\n";
    
    if (selected.lrc_url != "NOT_FOUND") {
        std::cout << "Lời bài hát: Có sẵn\n";
    } else {
        std::cout << "Lời bài hát: Không có\n";
    }

    std::cout << "\nBắt đầu phát nhạc...\n";
    if (player._play_url(selected.url)) {
        std::cout << "Đang phát: " << selected.title << " - " << selected.artist << "\n";
    } else {
        std::cout << "Lỗi khi phát nhạc\n";
        return 1;
    }

    // Control playback
    std::cout << "Nhấn Enter để tạm dừng...";
    std::cin.get();
    player.pause();

    std::cout << "\nNhấn Enter để tiếp tục...";
    std::cin.get();
    player.unpause();

    std::cout << "\nNhấn Enter để dừng và thoát...";
    std::cin.get();
    player.stop();

    return 0;
}