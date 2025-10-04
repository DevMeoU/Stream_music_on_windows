#pragma once

#include <string>
#include <map>
#include <utility>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
  #include <mmsystem.h>
  #pragma comment(lib, "winmm.lib")
#endif

// Struct cho gợi ý
struct SongSuggestion {
    std::string title;
    std::string artist;
    std::string url;
    std::string id;
    std::string lrc_url;  // Thêm URL lời bài hát
};

class MusicPlayer {
private:
    // trạng thái hiện tại
    std::string current_song;
    std::string current_url;
    bool        is_playing;
    bool        paused;

    // cấu hình API
    struct Config {
        std::string search_url;
        std::string play_url;
        std::string lyric_url;  // URL lấy lời bài hát
        std::map<std::string,std::string> headers;
    } config;

    // helper URL-encode
    static std::string url_encode(const std::string &value);

public:
    MusicPlayer();
    ~MusicPlayer();

    // Tìm với gợi ý top 3 (kết hợp tên bài + nghệ sĩ, parse JSON abslist với clean wrapped)
    std::vector<SongSuggestion> search_with_suggestions(const std::string &song_name, const std::string &artist_name = "");

    // Tìm 1 kết quả (fallback)
    std::pair<std::string,std::string> _search_song(const std::string &query);

    // stream & play bằng Windows MCI
    bool _play_url(const std::string &url);

    // Lấy lời bài hát
    std::string get_lyric(const std::string &song_id);

    // điều khiển playback
    void play();
    void pause();
    void unpause();
    void stop();

    // inline getters
    std::string get_current_song() const { return current_song; }
    bool        get_is_playing()   const { return is_playing; }
    bool        get_paused()       const { return paused; }
};