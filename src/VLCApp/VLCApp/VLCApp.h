// VLCApp.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>

// TODO: Reference additional headers your program requires here.
#ifdef _MSC_VER
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#include <winsock2.h>
#endif
#include <vlc/vlc.h>
#include <time.h>

#include <regex>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <curl_helper.h>
#include <iconv_helper.h>

class MediaObject {
public:
    MediaObject(uint64_t _time, libvlc_media_t* _media) :time(_time), pMedia(_media) {}
public:
    uint64_t time;
    libvlc_media_t* pMedia;
};
class AudioVideoMediaPlayer {
public:
    AudioVideoMediaPlayer() {
        // Load the VLC engine 
        m_pInstance = libvlc_new(0, nullptr);
    }
    ~AudioVideoMediaPlayer() {
        libvlc_release(m_pInstance);
    }
private:
    std::mutex m_lockMutex;
    libvlc_instance_t* m_pInstance = nullptr;
    libvlc_media_t* m_pMedia = nullptr;
    std::deque<MediaObject> m_pMediaQueue = {};
    std::deque<libvlc_media_t*> m_pMediaQueueHistory = {};
    libvlc_media_player_t* m_pMediaPlayer = nullptr;
public:
    void MediaAddOptions(const std::vector<std::string>& options)
    {
        for (auto& it : options)
        {
            libvlc_media_add_option(m_pMedia, it.c_str());
        }
    }
    void MediaAddOptionFlags(const std::unordered_map<std::string, unsigned>& optionflags)
    {
        for (auto& it : optionflags)
        {
            libvlc_media_add_option_flag(m_pMedia, it.first.c_str(), it.second);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //Create a new item
    //Method 1:
    //m = libvlc_media_new_location (pInstance, "file:///F:\\movie\\cuc_ieschool.flv");
    //Screen Capture
    //m = libvlc_media_new_location (pInstance, "screen://");
    //Method 2:
    //m = libvlc_media_new_path(pInstance, "trailer.mp4");
    //pMedia = libvlc_media_new_location(pInstance, "http://ivi.bupt.edu.cn/hls/cctv1.m3u8");
    void MediaStartUrl(const std::string& url)
    {
        m_pMedia = libvlc_media_new_location(m_pInstance, url.c_str());
    }
    void MediaStartPath(const std::string& path)
    {
        m_pMedia = libvlc_media_new_path(m_pInstance, path.c_str());
        libvlc_media_parse(m_pMedia);
        std::cout << libvlc_media_get_duration(m_pMedia) << " ms" << std::endl;
    }
    void MediaClose()
    {
        // No need to keep the media now
        libvlc_media_release(m_pMedia);
    }
    void MediaStartPathList(const std::unordered_map<uint64_t, std::string>& pathList)
    {
        std::unique_lock<std::mutex> lock(m_lockMutex);
        for (auto it : pathList)
        {
            m_pMediaQueue.emplace_back(MediaObject(it.first, libvlc_media_new_path(m_pInstance, it.second.c_str())));
            libvlc_media_parse(m_pMediaQueue.front().pMedia);
            //std::cout << libvlc_media_get_duration(m_pMediaQueue.front().pMedia) << " ms" << std::endl;
        }
    }
    void MediaCloseList()
    {
        for (auto it : m_pMediaQueue)
        {
            // No need to keep the media now
            libvlc_media_release(it.pMedia);
        }
    }
public:
    bool MediaPlayerQueueEmpty() {
        std::unique_lock<std::mutex> lock(m_lockMutex);
        return m_pMediaQueue.empty();
    }
    void MediaPlayerStart()
    {
        // Create a media player playing environement
        m_pMediaPlayer = libvlc_media_player_new(m_pInstance);
    }
    void MediaPlayerSetMedia()
    {
        libvlc_media_player_set_media(m_pMediaPlayer, m_pMedia);
    }
    std::string fileName;
    void MediaPlayerSetMediaQueue()
    {
        std::unique_lock<std::mutex> lock(m_lockMutex);
        if (!m_pMediaQueue.empty())
        {
            auto playItem = m_pMediaQueue.front();
            libvlc_media_player_set_media(m_pMediaPlayer, playItem.pMedia);
            libvlc_media_release(playItem.pMedia);
            fileName = std::to_string(playItem.time);
            m_pMediaQueue.pop_front();
        }
    }
    void MediaPlayerSetWindow(
#ifdef  _MSC_VER
        void*
#else
        int
#endif //  _MSC_VER
        window)
    {
#ifdef  _MSC_VER
        SetWindowTextA((HWND)window, fileName.c_str());
        libvlc_media_player_set_hwnd
#else
        libvlc_media_player_set_xwindow
#endif //  _MSC_VER
            (m_pMediaPlayer, window);
    }
    int MediaPlayerPlay()
    {
        // play the media_player
        return libvlc_media_player_play(m_pMediaPlayer);
    }
    int MediaPlayerWaitPlaying()
    {
        libvlc_state_t state = libvlc_NothingSpecial;
        do {
            state = libvlc_media_player_get_state(m_pMediaPlayer);
        } while (state != libvlc_Playing && state != libvlc_Error && state != libvlc_Ended);
        return libvlc_media_player_get_state(m_pMediaPlayer);
    }
    int MediaPlayerState()
    {
        return libvlc_media_player_get_state(m_pMediaPlayer);
    }
    void MediaPlayerPause(int no_pause)
    {
        // set pause the media_player
        libvlc_media_player_set_pause(m_pMediaPlayer, no_pause);
    }
    void MediaPlayerPause()
    {
        // pause the media_player
        libvlc_media_player_pause(m_pMediaPlayer);
    }
    int MediaPlayerCanPause()
    {
        // can pause the media_player
        return libvlc_media_player_can_pause(m_pMediaPlayer);
    }
    void MediaPlayerStop()
    {
        // stop the media_player
        libvlc_media_player_stop(m_pMediaPlayer);
    }
    void MediaPlayerClose()
    {
        // Free the media_player
        libvlc_media_player_release(m_pMediaPlayer);
    }
};
class MediaBlock {
public:
    MediaBlock(uint64_t _time, const std::string& _name) :time(_time), name(_name) {}
public:
    uint64_t time;
    std::string name;
};