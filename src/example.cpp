
#include "EncoderStreamer.h"
#include "lua_config.h"
#include <vector>
#include <memory>
#include <iostream>
#include <csignal>

std::atomic<bool> running(true);

void signal_handler(int signum) {
    running = false;
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    //v4l2-ctl -d /dev/video0 --list-formats-ext 查看摄像头支持格式
    std::vector<CameraConfig> camera_configs = read_camera_configs("../config/Config.lua");
    for (size_t i = 0; i < camera_configs.size(); ++i) {
            std::cout << "摄像头 " << i << ":" << std::endl;
            std::cout << "  device: " << camera_configs[i].device << std::endl;
            std::cout << "  rtmp_url: " << camera_configs[i].rtmp_url << std::endl;
            std::cout << "  分辨率: " << camera_configs[i].width << "x" << camera_configs[i].height << std::endl;
            std::cout << "  fps: " << camera_configs[i].fps << std::endl;
        }
    
    // 直接创建EncoderStreamer
    EncoderStreamer stream1(
        camera_configs[0].rtmp_url,
        camera_configs[0].device,
        camera_configs[0].width,
        camera_configs[0].height,
        camera_configs[0].fps,
        0  // camera_id
    );
    
    // EncoderStreamer stream2(
    //     camera_configs[1].rtmp_url,
    //     camera_configs[1].device,
    //     camera_configs[1].width,
    //     camera_configs[1].height,
    //     camera_configs[1].fps,
    //     1  // camera_id
    // );

    // 初始化并启动
    if (stream1.initialize(ModelType::Test, "../weight/rk3566/yolov5s_relu.rknn", 2, 2)) {
        std::cout << "stream1.initialize" << std::endl;
        stream1.start();
    }
    // if (stream2.initialize(ModelType::YoloV5, "../weight/rk3566/yolov5s_relu.rknn", 2, 2)) {
    //     stream2.start();
    // }


    while (running) {
        // 监控状态或处理其他任务
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 输出状态信息
        std::cout << "Running... (" << 2 << " streams active)" << std::endl;
    }

    stream1.stop();
    // stream2.stop();
    
    std::cout << "All streams stopped. Exiting." << std::endl;
    return 0;
}