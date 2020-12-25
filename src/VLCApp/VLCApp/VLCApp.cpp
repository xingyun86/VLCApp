// VLCApp.cpp : Defines the entry point for the application.
//

#include "VLCApp.h"
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
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <curl_helper.h>
#include <iconv_helper.h>

class MediaObject {
public:
    MediaObject(uint64_t _time, libvlc_media_t* _media):time(_time),pMedia(_media) {}
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
            std::cout << libvlc_media_get_duration(m_pMediaQueue.front().pMedia) << " ms" << std::endl;
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
        void * 
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
    MediaBlock(uint64_t _time, const std::string& _name):time(_time),name(_name) {}
public:
    uint64_t time;
    std::string name;
}; 
HWND hWnd = NULL;
static AudioVideoMediaPlayer avmp;
static std::deque<MediaBlock> mediaQueue;
std::mutex lockMutex;
std::condition_variable conditionVariable;
bool bRunning = true;
std::string urlPrefix = "http://ivi.bupt.edu.cn/hls/cctv2";
int main(int argc, char* argv[])
{
    std::cout << "Hello CMake." << std::endl;
    std::thread producerTask([]() 
        {
            while (bRunning)
            {
                CHttpTask rootTask;
                rootTask.GetTaskMap()->insert({ {0,CTaskInfoData(0, urlPrefix+".m3u8")} });
                rootTask.request();

                CHttpTask cacheTask;
                std::smatch sm = {};
                std::regex re(("-(.*?).ts\n"), std::regex::icase);
                std::string str = rootTask.GetTaskMap()->begin()->second.response_data;
                try
                {
                    std::unordered_map<uint64_t, std::string> playList;
                    for (auto its = str.cbegin(), ite = str.cend(); std::regex_search(its, ite, sm, re) == true; its = sm.begin()->second)
                    {
                        std::cout << urlPrefix + "-" + sm[1].str() + ".ts" << std::endl;
                        playList.emplace(std::stoull(sm[1].str()), urlPrefix + "-" + sm[1].str() + ".ts");
                    }
                    for (auto it = playList.begin(); it != playList.end(); it++)
                    {
                        cacheTask.GetTaskMap()->emplace(it->first, CTaskInfoData(it->first, it->second));
                    }
                    cacheTask.request();
                    for (auto it : *cacheTask.GetTaskMap())
                    {
                        auto fname = ("video/" + std::to_string(it.second.time) + ".ts");
                        if (access(fname.c_str(), 0) == (-1) && it.second.response_data.find("<html>") == std::string::npos)
                        {
                            std::unique_lock<std::mutex> lock(lockMutex);
                            std::cout << "producer" << std::endl;
                            FILE* pFile = fopen(fname.c_str(), "wb");
                            if (pFile)
                            {
                                fwrite(it.second.response_data.data(), it.second.response_data.size(), sizeof(char), pFile);
                                fclose(pFile);
                                mediaQueue.emplace_back(MediaBlock(it.second.time, fname));
                            }
                            conditionVariable.notify_all();
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    std::cout << e.what() << std::endl;
                }
                std::cout << time(nullptr) << "000" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }            
        }
    );
    std::thread consumerTask([]()
        {
            while (bRunning)
            {
                std::unique_lock<std::mutex> lock(lockMutex);
                std::cout << "consumer" << std::endl;
                conditionVariable.wait(lock, [] { return !mediaQueue.empty(); });
                std::unordered_map<uint64_t, std::string> vList;
                for (auto it : mediaQueue)
                {
                    vList.emplace(mediaQueue.front().time, mediaQueue.front().name);
                }
                mediaQueue.clear();
                avmp.MediaStartPathList(vList);
                conditionVariable.notify_all();
            }
        }
    );
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = (WNDPROC)[](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->LRESULT
    {
        switch (uMsg)
        {
        /*case WM_LBUTTONDOWN:
            MessageBoxA(hWnd, "您按下了鼠标左键昂", "豆豆的程序", MB_OK);
            HDC hdc;
            hdc = GetDC(hWnd);
            //The GetDC function retrieves a handle to a display device context for the client area of a specified window or for the entire screen. You can use the returned handle in subsequent GDI functions to draw in the device context.
            TextOutA(hdc, 0, 0, "感谢您对豆豆程序的支持昂", strlen("感谢您对豆豆程序的支持昂"));
            ReleaseDC(hWnd, hdc);
            break;
        case WM_CHAR:
            char szChar[20];
            sprintf(szChar, "Char is %d", wParam);
            MessageBoxA(hWnd, szChar, "豆豆的程序", MB_OK);
            break;
        case WM_PAINT:
            PAINTSTRUCT ps;
            HDC hDc;
            hDc = BeginPaint(hWnd, &ps);
            TextOutA(hDc, 0, 0, "这个是重绘滴哦", strlen("这个是重绘滴哦"));
            EndPaint(hWnd, &ps);
            break;
        case WM_CLOSE:
            if (IDYES == MessageBoxA(hWnd, "您真要退出么?", "程序", MB_YESNO))
            {
                DestroyWindow(hWnd);
            }
            break;*/
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
        return 0;
    };
    wcex.style = CS_HREDRAW | CS_VREDRAW;// redraw if size changes 
    wcex.cbClsExtra = 0;                // no extra class memory 
    wcex.cbWndExtra = 0;                // no extra window memory
    wcex.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.hInstance = GetModuleHandleA(NULL);
    wcex.lpszClassName = "VIDEO_CLASS";

    if (RegisterClassExA(&wcex)) 
    {
        MSG msg = { 0 };
        hWnd = CreateWindowExA(0, wcex.lpszClassName, wcex.lpszClassName, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, wcex.hInstance, NULL);
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
        
        avmp.MediaPlayerStart();

        /*avmp.MediaAddOptions({
            ":no-audio",
            ":video-title-position=2",
            ":sout-udp-caching=500",
            ":sout-rtp-caching=500",
            ":sout-mux-caching=500",
            ":sout-ts-dts-delay=500"
            });*/

        while (bRunning)
        {
            if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            else
            {
                int playState = avmp.MediaPlayerState();
                if (playState == libvlc_NothingSpecial || playState == libvlc_Ended)
                {
                    if (!avmp.MediaPlayerQueueEmpty())
                    {
                        //std::unique_lock<std::mutex> lock(lockMutex);
                        avmp.MediaPlayerSetMediaQueue();
                        avmp.MediaPlayerPlay();
                        avmp.MediaPlayerWaitPlaying();
                        avmp.MediaPlayerSetWindow(hWnd);
                        //conditionVariable.notify_all();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    bRunning = false;
    if(producerTask.joinable())
    {
        producerTask.join();
    }
    if (consumerTask.joinable())
    {
        consumerTask.join();
    }
    avmp.MediaPlayerStop();
    avmp.MediaPlayerClose();
    avmp.MediaClose();
    
    return 0;
}