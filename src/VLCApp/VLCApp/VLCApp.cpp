// VLCApp.cpp : Defines the entry point for the application.
//

#include "VLCApp.h"

HWND hWnd = NULL;
static AudioVideoMediaPlayer avmp;
static std::deque<MediaBlock> mediaQueue;
std::mutex lockMutex;
std::condition_variable conditionVariable;
bool bRunning = true;
std::string urlPrefix = "http://ivi.bupt.edu.cn/hls/cctv6";
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
                        //std::cout << urlPrefix + "-" + sm[1].str() + ".ts" << std::endl;
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
                            //std::cout << "producer" << std::endl;
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
                //std::cout << time(nullptr) << "000" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }            
        }
    );
    std::thread consumerTask([]()
        {
            while (bRunning)
            {
                std::unique_lock<std::mutex> lock(lockMutex);
                //std::cout << "consumer" << std::endl;
                conditionVariable.wait(lock, [] { return !mediaQueue.empty(); });
                std::unordered_map<uint64_t, std::string> vList;
                for (auto it : mediaQueue)
                {
                    vList.emplace(mediaQueue.front().time, mediaQueue.front().name);
                }
                mediaQueue.clear();
                avmp.MediaStartPathList(vList);
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
                    if (!avmp.MediaPlayerQueueEmpty())
                    {
                        avmp.MediaPlayerSetMediaQueue();
                        avmp.MediaPlayerPlay();
                        avmp.MediaPlayerWaitPlaying();
                        avmp.MediaPlayerSetWindow(hWnd);
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