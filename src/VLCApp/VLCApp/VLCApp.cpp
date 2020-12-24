// VLCApp.cpp : Defines the entry point for the application.
//

#include "VLCApp.h"
#ifdef _MSC_VER
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#include <windows.h>
#endif
#include <vlc/vlc.h>
#include <time.h>


int main(int argc, char* argv[])
{
    libvlc_instance_t* pInstance = nullptr;
    libvlc_media_t* pMedia = nullptr;
    libvlc_media_player_t* pMediaPlayer = nullptr;

    libvlc_time_t length = 0;
    int width = 0;
    int height = 0;
    int wait_time_ms = 1000;

    //libvlc_time_t length;

    // Load the VLC engine 
    pInstance = libvlc_new(0, nullptr);

    //Create a new item
    //Method 1:
    //m = libvlc_media_new_location (pInstance, "file:///F:\\movie\\cuc_ieschool.flv");
    //Screen Capture
    //m = libvlc_media_new_location (pInstance, "screen://");
    //Method 2:
    //m = libvlc_media_new_path(pInstance, "trailer.mp4");
    pMedia = libvlc_media_new_location(pInstance, "http://ivi.bupt.edu.cn/hls/cctv1.m3u8");

    const char* options[] = {
         //":no-audio",
         ":video-title-position=2",
         ":sout-udp-caching=500",
         ":sout-rtp-caching=500",
         ":sout-mux-caching=500",
         ":sout-ts-dts-delay=500"
    };
    for (int i = 0; i < sizeof(options) / sizeof(*options); i++)
    {
        libvlc_media_add_option(pMedia, options[i]);
    }
    // Create a media player playing environement
    pMediaPlayer = libvlc_media_player_new_from_media(pMedia);

    // No need to keep the media now
    libvlc_media_release(pMedia);

    // play the media_player
    libvlc_media_player_play(pMediaPlayer);

    //wait until the tracks are created
    while (length == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
        length = libvlc_media_player_get_length(pMediaPlayer);
        width = libvlc_video_get_width(pMediaPlayer);
        height = libvlc_video_get_height(pMediaPlayer);
        printf("Stream Duration: %ds\n", (int)length / 1000);
        printf("Resolution: %d x %d\n", width, height);
    }
    //Let it play 
    std::this_thread::sleep_for(std::chrono::milliseconds(length - wait_time_ms));

    // Stop playing
    libvlc_media_player_stop(pMediaPlayer);

    // Free the media_player
    libvlc_media_player_release(pMediaPlayer);

    libvlc_release(pInstance);

    return 0;
}
/*int main(int argc, char* argv[])
{
    libvlc_instance_t* inst;
    libvlc_media_player_t* mp;
    libvlc_media_t* m;

    libvlc_time_t length;
    int width;
    int height;
    int wait_time = 5000;

    //libvlc_time_t length;

    // Load the VLC engine 
    inst = libvlc_new(0, NULL);

    //Create a new item
    //Method 1:
    //m = libvlc_media_new_location (inst, "file:///F:\\movie\\cuc_ieschool.flv");
    //Screen Capture
    //m = libvlc_media_new_location (inst, "screen://");
    //Method 2:
    //m = libvlc_media_new_path(inst, "trailer.mp4");
    m = libvlc_media_new_location(inst, "http://ivi.bupt.edu.cn/hls/cctv1.m3u8");

    // Create a media player playing environement
    mp = libvlc_media_player_new_from_media(m);

    // No need to keep the media now
    libvlc_media_release(m);

    // play the media_player
    libvlc_media_player_play(mp);

    //wait until the tracks are created
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    length = libvlc_media_player_get_length(mp);
    width = libvlc_video_get_width(mp);
    height = libvlc_video_get_height(mp);
    printf("Stream Duration: %ds\n", (int)length / 1000);
    printf("Resolution: %d x %d\n", width, height);
    //Let it play 
    std::this_thread::sleep_for(std::chrono::milliseconds(length - wait_time));

    // Stop playing
    libvlc_media_player_stop(mp);

    // Free the media_player
    libvlc_media_player_release(mp);

    libvlc_release(inst);

    return 0;
}*/
/*int main(int argc, char* argv[])
{
    std::cout << "Hello CMake." << std::endl;
    libvlc_instance_t* vlc_ins = NULL;
    libvlc_media_player_t* vlc_player = NULL;
    libvlc_media_t* vlc_media = NULL;

    const char* vlc_args[] =
    {
        "-I",
        "dummy",
        "--ignore-config",
        "--extraintf=logger",
        "--verbose=2",
    };

    // 创建一个VLC实例
    vlc_ins = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    if (vlc_ins != NULL)
    {
        // 创建一个VLC播放器
        vlc_player = libvlc_media_player_new(vlc_ins);
        if (vlc_player != NULL)
        {
            // 通过文件路径创建一个媒体实例,这里是我的测试文件
            vlc_media = libvlc_media_new_path(vlc_ins, "trailer.mp4");
            if (vlc_media != NULL)
            {
                // 解析媒体实例
                libvlc_media_parse(vlc_media);
                // 获取媒体文件的播放长度,  返回 ms
                libvlc_time_t duration = libvlc_media_get_duration(vlc_media);

                // 此处是获取媒体包含多个的视频和音频轨以及其他类型的轨道信息
                libvlc_media_track_info_t* media_tracks = NULL;
                int trackCount = libvlc_media_get_tracks_info(vlc_media, &media_tracks);
                // 这里是释放内存，但我测试的时候会有问题，还没仔细研究是为何
                // free(media_tracks);  // crash?

                // 把打开的媒体文件设置给播放器
                libvlc_media_player_set_media(vlc_player, vlc_media);

                // 因为是windows系统，所以需要设置一个HWND给播放器作为窗口,这里就直接使用桌面窗口,这里仅是测试
                libvlc_media_player_set_hwnd(vlc_player, ::GetDesktopWindow());
                // 开始播放视频
                libvlc_media_player_play(vlc_player);

                // 这里仅是为了打印一些信息出来,20秒后退出
                time_t last_time = time(NULL);
                while ((time(NULL) < (last_time + 200)))
                {
                    Sleep(10);
                    // 获取当前播放位置
                    libvlc_time_t play_time = libvlc_media_player_get_time(vlc_player);
                    printf("playing time : %lld ms\r", (__int64)(play_time));

                    // 获取媒体的状态
                    // libvlc_state_t media_state = libvlc_media_get_state(vlc_media);
                    // printf("\nmedia state : %d\n", (int)(media_state));
                }
                // 停止
                libvlc_media_player_stop(vlc_player);
                // 释放
                libvlc_media_release(vlc_media);
            }
            // 释放
            libvlc_media_player_release(vlc_player);
        }
        // 释放
        libvlc_release(vlc_ins);
    }
    return 0;
}*/