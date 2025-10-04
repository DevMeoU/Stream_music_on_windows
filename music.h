/* 
 * Author: Deesol
 * Create: TODAY
 */

#ifndef __MUSIC_H__
#define __MUSIC_H__

/* **************************************************
 *      Include
 * **************************************************/
#include <string>
#include <vector>
#include <utility> // std::pair

 /* **************************************************
 *      Definitions
 * **************************************************/
class MusicMetadata {
private:
public:
    MusicMetadata();
    ~MusicMetadata();

    virtual bool extract_metadata();
    virtual void _get_tag_value();    
    virtual void format_duration();
};

/* **************************************************
*      Class
* **************************************************/


class MusicPlayer {
private:
    std::string current_song;
    std::string current_url;
    std::string current_song_id;
    int total_duration;
    bool is_playing;
    bool is_paused;
    int current_position;
    int start_play_time;

    std::vector<std::string> lyrics;
    int current_lyric_index;

    std::string cache_dir;
    struct config {
        std::string search_url;
        std::string play_url;
        std::string lyrics_url;
    } config;
public:
    MusicPlayer();
    ~MusicPlayer();

    virtual void _init_pygame_mixer();
    virtual void _initialize_app_reference();    
    virtual void _init_cache_dirs();
    virtual void _scan_local_music();
    virtual void get_local_playlist();
    virtual void search_local_music();
    virtual void play_local_song_by_id();
    virtual void get_current_song();
    virtual void get_is_playing();
    virtual void get_paused();
    virtual void get_duration();
    virtual void get_position();
    virtual void get_progress();
    virtual void _handle_playback_finished();

    virtual void play();
    virtual void pause();
    virtual void unpause();
    virtual void stop();
    virtual void next();
    virtual void previous();
    virtual void set_volume();
    virtual void seek();

    virtual std::pair<std::string, std::string> _search_song(const std::string& song_name);
    virtual void _play_url();
    virtual void _get_or_download_file();
    virtual void _download_file();
    virtual void _fetch_lyrics();
    virtual void _lyrics_update_task();
    virtual void _find_current_lyric_index();
    virtual void  _display_current_lyric();
    virtual void _extract_value();
    virtual void _format_time();
    virtual void _safe_update_ui();
}

 /* **************************************************
 *      Prototype
 * **************************************************/



#endif /* __MUSIC_H__ */