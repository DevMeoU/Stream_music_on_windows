#include "esp_32_music.h"
#include <iostream>
#include <curl/curl.h>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>  // JSON lib
#include <regex>  // Cho extract thủ công

using json = nlohmann::json;

// libcurl write callback
static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* out = static_cast<std::string*>(userdata);
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// URL-encode implementation
std::string MusicPlayer::url_encode(const std::string &value) {
    std::ostringstream esc;
    esc.fill('0');
    esc << std::hex;
    for (unsigned char c : value) {
        if (std::isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') {
            esc << c;
        } else {
            esc << '%' << std::uppercase << std::setw(2)
                << int(c) << std::nouppercase;
        }
    }
    return esc.str();
}

// Constructor: khởi tạo config
MusicPlayer::MusicPlayer()
  : current_song(""),
    current_url(""),
    is_playing(false),
    paused(false)
{
    config.search_url = "http://search.kuwo.cn/r.s";
    config.play_url   = "https://api.xiaodaokg.com/kuwo.php";
    config.lyric_url  = "https://api.xiaodaokg.com/kw/kwlyric.php";
    config.headers = {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"},
        {"Accept",     "application/json"},
        {"Connection", "keep-alive"},
        {"Referer",    "http://www.kuwo.cn/"}
    };
    std::cout << "[INIT] MusicPlayer khởi tạo\n";
}

// Destructor: dừng & đóng alias mp3
MusicPlayer::~MusicPlayer() {
    stop();
    std::cout << "[DESTROY] MusicPlayer hủy\n";
}

// Lấy lời bài hát
std::string MusicPlayer::get_lyric(const std::string &song_id) {
    if (song_id.empty()) {
        std::cout << "[WARNING] Không có ID bài hát để lấy lời\n";
        return "NOT_FOUND";
    }

    CURL* curl = curl_easy_init();
    std::string lyric_response;
    if (!curl) {
        std::cerr << "[ERROR] curl init thất bại\n";
        return "NOT_FOUND";
    }

    std::string lyric_url = config.lyric_url + "?id=" + song_id;
    std::cout << "[INFO] Lấy URL lời bài hát: " << lyric_url << "\n";

    curl_easy_setopt(curl, CURLOPT_URL, lyric_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &lyric_response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* hdr = NULL;
    for (auto &h : config.headers) {
        hdr = curl_slist_append(hdr, (h.first + ": " + h.second).c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl perform fail: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(hdr);
        return "NOT_FOUND";
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "[INFO] Link response: <Response [" << http_code << "]>\n";

    curl_easy_cleanup(curl);
    curl_slist_free_all(hdr);

    if (lyric_response.empty() || lyric_response.find("NOT_FOUND") != std::string::npos) {
        std::cout << "[WARNING] Không lấy được lời bài hát hoặc định dạng lời bài hát sai: NOT_FOUND\n";
        return "NOT_FOUND";
    }

    return lyric_response;
}

// search_with_suggestions: Tìm với gợi ý top 3, parse JSON abslist
std::vector<SongSuggestion> MusicPlayer::search_with_suggestions(const std::string &song_name, const std::string &artist_name) {
    std::string query = song_name;
    if (!artist_name.empty()) {
        query += " " + artist_name;
    }
    std::cout << "[INFO] Tham số tìm kiếm: {'all': '" << query 
              << "', 'ft': 'music', 'newsearch': '1', 'alflac': '1', "
              << "'itemset': 'web_2013', 'client': 'kt', 'cluster': '0', "
              << "'pn': '0', 'rn': '1', 'vermerge': '1', 'rformat': 'json', "
              << "'encoding': 'utf8', 'show_copyright_off': '1', 'pcmp4': '1', "
              << "'ver': 'mbox', 'vipver': 'MUSIC_8.7.6.0.BCS31', 'plat': 'pc', 'devid': '0'}\n";

    CURL* curl = curl_easy_init();
    std::string rawResponse;
    if (!curl) {
        std::cerr << "[ERROR] curl init thất bại\n";
        return {};
    }

    // URL với các tham số giống log mẫu
    std::ostringstream ss;
    ss << config.search_url
       << "?all=" << url_encode(query)
       << "&ft=music&newsearch=1&alflac=1"
       << "&itemset=web_2013&client=kt&cluster=0"
       << "&pn=0&rn=1&vermerge=1&rformat=json"
       << "&encoding=utf8&show_copyright_off=1&pcmp4=1"
       << "&ver=mbox&vipver=MUSIC_8.7.6.0.BCS31&plat=pc&devid=0";
    std::string searchUrl = ss.str();

    curl_easy_setopt(curl, CURLOPT_URL, searchUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawResponse);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* hdr = NULL;
    for (auto &h : config.headers) {
        hdr = curl_slist_append(hdr, (h.first + ": " + h.second).c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);

    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "[INFO] Link response: <Response [" << http_code << "]>\n";

    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl perform fail: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(hdr);
        return {};
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdr);

    if (rawResponse.empty()) {
        std::cerr << "[ERROR] Response rỗng\n";
        return {};
    }

    std::vector<SongSuggestion> suggestions;
    try {
        // Parse JSON response
        json j = json::parse(rawResponse);
        json abslist;

        if (j.contains("abslist")) {
            abslist = j["abslist"];
        } else if (j.contains("data") && j["data"].contains("abslist")) {
            abslist = j["data"]["abslist"];
        } else {
            throw json::parse_error::create(102, 0, "No abslist found in JSON", nullptr);
        }

        for (auto& song : abslist) {
            SongSuggestion sug;
            sug.id = song.contains("DC_TARGETID") ? song["DC_TARGETID"].get<std::string>() : "";
            sug.title = song.contains("SONGNAME") ? song["SONGNAME"].get<std::string>() : "Unknown Title";
            sug.artist = song.contains("SINGER") ? song["SINGER"].get<std::string>() : "Unknown Artist";

            // Lấy URL mp3 từ ID
            if (!sug.id.empty()) {
                std::string proxyUrl = config.play_url + "?ID=" + sug.id;
                CURL* curl_proxy = curl_easy_init();
                std::string mp3_url;
                if (curl_proxy) {
                    curl_easy_setopt(curl_proxy, CURLOPT_URL, proxyUrl.c_str());
                    curl_easy_setopt(curl_proxy, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl_proxy, CURLOPT_WRITEDATA, &mp3_url);
                    curl_easy_setopt(curl_proxy, CURLOPT_TIMEOUT, 10L);
                    curl_easy_setopt(curl_proxy, CURLOPT_FOLLOWLOCATION, 1L);

                    struct curl_slist* proxy_hdr = NULL;
                    for (auto &h : config.headers) {
                        proxy_hdr = curl_slist_append(proxy_hdr, (h.first + ": " + h.second).c_str());
                    }
                    curl_easy_setopt(curl_proxy, CURLOPT_HTTPHEADER, proxy_hdr);

                    CURLcode res_proxy = curl_easy_perform(curl_proxy);
                    if (res_proxy != CURLE_OK) {
                        std::cerr << "[ERROR] Proxy fail for ID=" << sug.id << ": " << curl_easy_strerror(res_proxy) << "\n";
                    } else if (mp3_url.empty()) {
                        std::cerr << "[ERROR] Empty mp3 URL for ID=" << sug.id << "\n";
                    } else {
                        sug.url = mp3_url;
                        // Lấy lời bài hát
                        sug.lrc_url = get_lyric(sug.id);
                    }
                    curl_easy_cleanup(curl_proxy);
                    curl_slist_free_all(proxy_hdr);
                }
            }

            std::cout << "[INFO] Tìm thấy bài hát ID: " << sug.id << ", URL: " << sug.url << "\n";
            suggestions.push_back(sug);
            break; // Chỉ lấy 1 kết quả như log mẫu
        }

        // Gán current_song và current_url nếu có kết quả
        if (!suggestions.empty()) {
            current_song = suggestions[0].title;
            current_url = suggestions[0].url;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Lỗi xử lý JSON: " << e.what() << "\n";
        // Fallback parsing
        std::string songId, title, artist;
        size_t p_id = rawResponse.find("\"DC_TARGETID\":\"");
        if (p_id == std::string::npos) {
            p_id = rawResponse.find("'DC_TARGETID':'");
        }
        if (p_id != std::string::npos) {
            p_id += (rawResponse[p_id] == '"' ? strlen("\"DC_TARGETID\":\"") : strlen("'DC_TARGETID':'"));
            size_t e_id = rawResponse.find(rawResponse[p_id-1], p_id);
            songId = rawResponse.substr(p_id, e_id - p_id);

            // Lấy URL mp3 từ ID (fallback)
            std::string proxyUrl = config.play_url + "?ID=" + songId;
            CURL* curl_proxy = curl_easy_init();
            std::string mp3_url;
            if (curl_proxy) {
                curl_easy_setopt(curl_proxy, CURLOPT_URL, proxyUrl.c_str());
                curl_easy_setopt(curl_proxy, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl_proxy, CURLOPT_WRITEDATA, &mp3_url);
                curl_easy_setopt(curl_proxy, CURLOPT_TIMEOUT, 10L);
                curl_easy_setopt(curl_proxy, CURLOPT_FOLLOWLOCATION, 1L);

                struct curl_slist* proxy_hdr = NULL;
                for (auto &h : config.headers) {
                    proxy_hdr = curl_slist_append(proxy_hdr, (h.first + ": " + h.second).c_str());
                }
                curl_easy_setopt(curl_proxy, CURLOPT_HTTPHEADER, proxy_hdr);

                CURLcode res_proxy = curl_easy_perform(curl_proxy);
                if (res_proxy == CURLE_OK && !mp3_url.empty()) {
                    SongSuggestion sug;
                    sug.id = songId;
                    sug.title = "Unknown Title";
                    sug.artist = "Unknown Artist";
                    sug.url = mp3_url;
                    sug.lrc_url = get_lyric(songId);
                    suggestions.push_back(sug);
                    std::cout << "[INFO] Fallback - Tìm thấy bài hát ID: " << sug.id << ", URL: " << sug.url << "\n";
                }
                curl_easy_cleanup(curl_proxy);
                curl_slist_free_all(proxy_hdr);
            }
        }
    }

    if (suggestions.empty()) {
        std::cerr << "[ERROR] Không tìm thấy bài hát nào phù hợp\n";
    }
    return suggestions;
}

// _search_song: Fallback cải tiến
std::pair<std::string,std::string> MusicPlayer::_search_song(const std::string &query) {
    std::cout << "[DEBUG] _search_song với: " << query << "\n";
    CURL* curl = curl_easy_init();
    std::string rawSearch;
    if (!curl) {
        std::cerr << "[ERROR] curl init thất bại\n";
        return {"", ""};
    }

    std::ostringstream ss;
    ss << config.search_url
       << "?all=" << url_encode(query)
       << "&ft=music&newsearch=1&alflac=1"
       << "&itemset=web_2013&client=kt&cluster=0"
       << "&pn=0&rn=1&vermerge=1&rformat=json"
       << "&encoding=utf8&show_copyright_off=1&pcmp4=1"
       << "&ver=mbox&vipver=MUSIC_8.7.6.0.BCS31&plat=pc&devid=0";
    std::string searchUrl = ss.str();

    curl_easy_setopt(curl, CURLOPT_URL, searchUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawSearch);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* hdr = NULL;
    for (auto &h : config.headers) {
        hdr = curl_slist_append(hdr, (h.first + ": " + h.second).c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl perform fail: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(hdr);
        return {"", ""};
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdr);

    std::cout << "[DEBUG] Fallback rawSearch (size=" << rawSearch.size() << "): " << rawSearch.substr(0, 200) << (rawSearch.size() > 200 ? "..." : "") << "\n";

    std::string songId, title, artist;
    size_t p = rawSearch.find("\"DC_TARGETID\":\"");
    char d = '"';
    if (p == std::string::npos) {
        p = rawSearch.find("'DC_TARGETID':'");
        d = '\'';
    }
    if (p != std::string::npos) {
        p += (d == '"' ? strlen("\"DC_TARGETID\":\"") : strlen("'DC_TARGETID':'"));
        size_t e = rawSearch.find(d, p);
        songId = rawSearch.substr(p, e - p);
    } else {
        std::cerr << "[ERROR] Không tìm DC_TARGETID\n";
        return {"", ""};
    }
    std::cout << "[DEBUG] song_id=" << songId << "\n";

    size_t p_title = rawSearch.find("\"SONGNAME\":\"", p);
    if (p_title == std::string::npos) {
        p_title = rawSearch.find("'SONGNAME':'", p);
    }
    if (p_title != std::string::npos) {
        p_title += (rawSearch[p_title] == '"' ? strlen("\"SONGNAME\":\"") : strlen("'SONGNAME':'"));
        size_t e_title = rawSearch.find(rawSearch[p_title-1], p_title);
        title = rawSearch.substr(p_title, e_title - p_title);
    } else {
        title = query;
    }

    size_t p_artist = rawSearch.find("\"SINGER\":\"", p_title);
    if (p_artist == std::string::npos) {
        p_artist = rawSearch.find("'SINGER':'", p_title);
    }
    if (p_artist != std::string::npos) {
        p_artist += (rawSearch[p_artist] == '"' ? strlen("\"SINGER\":\"") : strlen("'SINGER':'"));
        size_t e_artist = rawSearch.find(rawSearch[p_artist-1], p_artist);
        artist = rawSearch.substr(p_artist, e_artist - p_artist);
    } else {
        artist = "Unknown Artist";
    }

    curl = curl_easy_init();
    std::string rawProxy;
    std::string proxyUrl = config.play_url + "?ID=" + songId;
    if (!curl) {
        std::cerr << "[ERROR] curl init thất bại\n";
        return {songId, ""};
    }

    curl_easy_setopt(curl, CURLOPT_URL, proxyUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawProxy);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* proxy_hdr = NULL;
    for (auto &h : config.headers) {
        proxy_hdr = curl_slist_append(proxy_hdr, (h.first + ": " + h.second).c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, proxy_hdr);

    CURLcode res_proxy = curl_easy_perform(curl);
    if (res_proxy != CURLE_OK) {
        std::cerr << "[ERROR] Proxy fail: " << curl_easy_strerror(res_proxy) << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(proxy_hdr);
        return {songId, ""};
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(proxy_hdr);

    if (rawProxy.empty()) {
        std::cerr << "[ERROR] Proxy không trả URL mp3\n";
        return {songId, ""};
    }

    current_song = title;
    current_url  = rawProxy;
    std::cout << "[DEBUG] URL mp3=" << rawProxy << "\n";
    return {songId, rawProxy};
}

// _play_url: Kiểm tra lỗi MCI
bool MusicPlayer::_play_url(const std::string &url) {
#ifdef _WIN32
    if (url.empty()) {
        std::cerr << "[ERROR] URL mp3 rỗng\n";
        return false;
    }

    mciSendStringA("stop mp3",  NULL, 0, NULL);
    mciSendStringA("close mp3", NULL, 0, NULL);

    std::string cmd = "open \"" + url + "\" type mpegvideo alias mp3";
    DWORD result = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    if (result != 0) {
        char error[128];
        mciGetErrorStringA(result, error, sizeof(error));
        std::cerr << "[ERROR] MCI open fail: " << error << "\n";
        return false;
    }

    result = mciSendStringA("play mp3", NULL, 0, NULL);
    if (result != 0) {
        char error[128];
        mciGetErrorStringA(result, error, sizeof(error));
        std::cerr << "[ERROR] MCI play fail: " << error << "\n";
        mciSendStringA("close mp3", NULL, 0, NULL);
        return false;
    }

    is_playing = true;
    paused     = false;
    std::cout << "[play] Đang phát stream: " << url << "\n";
    return true;
#else
    std::cerr << "[ERROR] Chỉ hỗ trợ Windows MCI playback\n";
    return false;
#endif
}

void MusicPlayer::play() {
#ifdef _WIN32
    DWORD result = mciSendStringA("play mp3", NULL, 0, NULL);
    if (result != 0) {
        char error[128];
        mciGetErrorStringA(result, error, sizeof(error));
        std::cerr << "[ERROR] MCI play fail: " << error << "\n";
    } else {
        is_playing = true;
        paused = false;
    }
#endif
}

void MusicPlayer::pause() {
#ifdef _WIN32
    DWORD result = mciSendStringA("pause mp3", NULL, 0, NULL);
    if (result != 0) {
        char error[128];
        mciGetErrorStringA(result, error, sizeof(error));
        std::cerr << "[ERROR] MCI pause fail: " << error << "\n";
    } else {
        paused = true;
    }
#endif
}

void MusicPlayer::unpause() {
#ifdef _WIN32
    DWORD result = mciSendStringA("resume mp3", NULL, 0, NULL);
    if (result != 0) {
        char error[128];
        mciGetErrorStringA(result, error, sizeof(error));
        std::cerr << "[ERROR] MCI resume fail: " << error << "\n";
    } else {
        paused = false;
    }
#endif
}

void MusicPlayer::stop() {
#ifdef _WIN32
    mciSendStringA("stop mp3",  NULL, 0, NULL);
    mciSendStringA("close mp3", NULL, 0, NULL);
    is_playing = false;
    paused = false;
#endif
}