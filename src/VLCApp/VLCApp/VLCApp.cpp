// VLCApp.cpp : Defines the entry point for the application.
//

#include "VLCApp.h"

HWND hWnd = NULL;
HWND hWndListBox = NULL;
HWND hWndStatic = NULL;
static AudioVideoMediaPlayer avmp;
static std::deque<MediaBlock> mediaQueue;
std::mutex lockMutex;
std::condition_variable conditionVariable;
bool bRunning = true;
ppsyqm::json jsonConfig;
std::unordered_map<std::string, std::string> videoList;
std::string urlPrefix = "";// "http://ivi.bupt.edu.cn/hls/cctv6hd";

CHttpTask cacheTask;

DWORD dwListBoxStyle = WS_TABSTOP |
WS_CHILD |
WS_BORDER |
WS_VISIBLE |
LBS_STANDARD;
DWORD dwStaticStyle = WS_TABSTOP |
WS_CHILD |
WS_BORDER |
WS_VISIBLE |
ES_CENTER;
#define LISTBOX_ID 10001
#define STATIC_ID  10002

int main(int argc, char* argv[])
{
    std::cout << "Hello CMake." << std::endl;
    try
    {
        jsonConfig = ppsyqm::json::parse(FILE_READER("config.json", std::ios::binary));
        if (jsonConfig.is_object())
        {
            if (!jsonConfig["list"].is_array())
            {
                return 0;
            }
            for (auto it : jsonConfig["list"])
            {
                if (it["name"].is_string() && it["url"].is_string())
                {
                    videoList.emplace(u2g(it["name"].get<std::string>()).c_str(), u2g(it["url"].get<std::string>()).c_str());
                }
            }
        }
    }
    catch (const std::exception&e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }
    std::thread producerTask([]() 
        {
            while (bRunning)
            {
                if (urlPrefix.length() <= 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                cacheTask.GetTaskMap()->clear();
                cacheTask.GetTaskMap()->insert({ {0,CTaskInfoData(0, urlPrefix + ".m3u8")} });
                cacheTask.request();

                std::smatch sm = {};
                std::regex re(("-(.*?).ts\n"), std::regex::icase);
                std::string str = cacheTask.GetTaskMap()->begin()->second.response_data;
                if (str.length() <= 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                try
                {
                    std::unordered_map<uint64_t, std::string> playList;
                    for (auto its = str.cbegin(), ite = str.cend(); std::regex_search(its, ite, sm, re) == true; its = sm.begin()->second)
                    {
                        //std::cout << urlPrefix + "-" + sm[1].str() + ".ts" << std::endl;
                        playList.emplace(std::stoull(sm[1].str()), urlPrefix + "-" + sm[1].str() + ".ts");
                    }
                    cacheTask.GetTaskMap()->clear();
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
                            std::cout << "enter producer" << std::endl;
                            FILE* pFile = fopen(fname.c_str(), "wb");
                            if (pFile)
                            {
                                fwrite(it.second.response_data.data(), it.second.response_data.size(), sizeof(char), pFile);
                                fclose(pFile);
                                mediaQueue.emplace_back(MediaBlock(it.second.time, fname));
                            }
                            conditionVariable.notify_all();
                            std::cout << "leave producer" << std::endl;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    std::cout << e.what() << std::endl;
                }
                //std::cout << time(nullptr) << "000" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }            
        }
    );
    std::thread consumerTask([]()
        {
            while (bRunning)
            {
                std::unique_lock<std::mutex> lock(lockMutex);
                std::cout << "enter consumer" << std::endl;
                conditionVariable.wait(lock, [] { return !mediaQueue.empty(); });
                std::unordered_map<uint64_t, std::string> vList;
                for (auto it : mediaQueue)
                {
                    vList.emplace(mediaQueue.front().time, mediaQueue.front().name);
                }
                mediaQueue.clear();
                avmp.MediaStartPathList(vList);
                std::cout << "leave consumer" << std::endl;
            }
        }
    );
    InitCommonControls();
    INITCOMMONCONTROLSEX iccex = { 0 };
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = 
        ICC_LISTVIEW_CLASSES |
        ICC_TREEVIEW_CLASSES |
        ICC_BAR_CLASSES |
        ICC_TAB_CLASSES |
        ICC_UPDOWN_CLASS |
        ICC_PROGRESS_CLASS |
        ICC_HOTKEY_CLASS |
        ICC_ANIMATE_CLASS |
        ICC_WIN95_CLASSES |
        ICC_DATE_CLASSES |
        ICC_USEREX_CLASSES |
        ICC_COOL_CLASSES |
        ICC_INTERNET_CLASSES |
        ICC_PAGESCROLLER_CLASS |
        ICC_NATIVEFNTCTL_CLASS |
        ICC_STANDARD_CLASSES |
        ICC_LINK_CLASS;
    InitCommonControlsEx(&iccex);
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
        case WM_CREATE:
        {
            hWndListBox = CreateWindowExA(WS_EX_CLIENTEDGE,          // ex style
                WC_LISTBOXA,               // class name - defined in commctrl.h
                (""),                        // dummy text
                dwListBoxStyle,                   // style
                0,                         // x position
                0,                         // y position
                0,                         // width
                0,                         // height
                hWnd,                // parent
                (HMENU)LISTBOX_ID,        // ID
                GetModuleHandleA(NULL),                   // instance
                NULL);                     // no extra data
            hWndStatic = CreateWindowExA(WS_EX_CLIENTEDGE,          // ex style
                WC_EDITA,               // class name - defined in commctrl.h
                ("123"),                        // dummy text
                dwStaticStyle,                   // style
                0,                         // x position
                0,                         // y position
                0,                         // width
                0,                         // height
                hWnd,                // parent
                (HMENU)STATIC_ID,        // ID
                GetModuleHandleA(NULL),                   // instance
                NULL);
            for (auto it : videoList)
            {
                SendMessageA(hWndListBox, LB_INSERTSTRING, (WPARAM)SendMessageA(hWndListBox, LB_GETCOUNT, (WPARAM)0, (LPARAM)0), (LPARAM)it.first.c_str());
            }

            avmp.MediaPlayerStart();

            /*avmp.MediaAddOptions({
                ":no-audio",
                ":video-title-position=2",
                ":sout-udp-caching=500",
                ":sout-rtp-caching=500",
                ":sout-mux-caching=500",
                ":sout-ts-dts-delay=500"
                });*/

        }
        break;
        case WM_COMMAND:
        {
            switch(LOWORD(wParam)) 
            {
            case LISTBOX_ID:
            {
                switch (HIWORD(wParam))
                {
                case LBN_DBLCLK:
                {
                    CHAR szText[MAX_PATH] = { 0 };
                    SendMessageA(hWndListBox, LB_GETTEXT, SendMessageA(hWndListBox, LB_GETCURSEL, 0, 0), (LPARAM)szText);
                    urlPrefix = videoList.at(szText);
                    cacheTask.SetProcessing(0);
                    avmp.MediaPlayerSetPause(0);
                    avmp.MediaPlayerQueueClear();
                    cacheTask.SetProcessing(1);
                }
                    break;
                }
            }
            break;
            }
        }
        break;
        case WM_SIZE:
        {
            if (hWnd != NULL && hWndListBox != NULL && hWndStatic != NULL)
            {
                RECT rcWnd = { 0 };
                GetClientRect(hWnd, &rcWnd);
                INT nWidth = (rcWnd.right - rcWnd.left);
                INT nHeidht = (rcWnd.bottom - rcWnd.top);
                MoveWindow(hWndListBox, 0, 0, nWidth / 3, nHeidht, FALSE);
                MoveWindow(hWndStatic, nWidth / 3, 0, nWidth - nWidth / 3, nHeidht, FALSE);
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
            }
        }
        break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
        default:
        {
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
        break;
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
                std::unordered_map<std::string, std::string> fileList;
                enum_file(fileList, "video");
                for (auto it : fileList)
                {
                    //std::cout << it.first << "," << it.second << std::endl;
                    if (it.second.ends_with(".ts"))
                    {
                        uint64_t itemTime = std::stoull(it.second.substr(0, it.second.length()-strlen(".ts")));
                        if (itemTime > 0ULL && avmp.playtime > itemTime)
                        {
                            system(("del /s /q " + it.first).c_str());
                        }
                    }
                }
                int playState = avmp.MediaPlayerState();
                if (playState == libvlc_NothingSpecial || playState == libvlc_Ended)
                {
                    if (!avmp.MediaPlayerQueueEmpty() && hWndStatic != NULL)
                    {
                        avmp.MediaPlayerSetMediaQueue();
                        avmp.MediaPlayerPlay();
                        avmp.MediaPlayerWaitPlaying();
                        avmp.MediaPlayerSetWindow(hWndStatic);
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