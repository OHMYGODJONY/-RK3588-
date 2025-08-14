#pragma once
/**
 * @file EncoderStreamer.h
 * @author achene
 * @date 2025-08-05
 * @class EncoderStreamer
 * @brief 视频编码推流器，实现从摄像头帧到RTMP流的完整处理流程
 * 
 * 功能包括：初始化FFmpeg编码器、启动/停止编码线程、接收摄像头帧、处理帧数据、
 * 编码为H.264等格式并推流至指定的RTMP服务器。支持自定义图像处理逻辑。
 * 
 * 该类整合了图像处理、FFmpeg编码以及RTMP推流功能，通过多线程实现帧处理与编码推流的异步操作，
 * 支持设置自定义图像处理处理器，适用于实时视频流传输场景。
 */

#include "thread_safe_queue.h"
#include "CameraCapture.h"
#include "Model.h"
#include "ModelFactory.h"
#include "threadpool.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
}



class EncoderStreamer {
public:
    /**
     * @brief 构造函数，初始化编码器推流器基本参数
     * @param rtmp_url RTMP服务器地址
     * @param width 视频宽度
     * @param height 视频高度
     * @param fps 视频帧率
     * @param bitrate 视频比特率，默认值为2000000
     */
    EncoderStreamer(const std::string& rtmp_url, 
                const std::string& device_path, 
                int width, 
                int height, 
                int fps,
                int camera_id,
                uint32_t pixel_format = V4L2_PIX_FMT_YUYV,
                int bitrate = 2000000);

    /**
     * @brief 析构函数，释放资源
     */
    ~EncoderStreamer();

    /**
     * @brief 初始化编码器和推流相关资源
     * @return 初始化成功返回true，否则返回false
     */
    bool initialize(ModelType model_type, const std::string& model_path, 
                   int thread_count, int model_pool_size);

    /**
     * @brief 启动编码推流线程
     */
    void start();

    /**
     * @brief 停止编码推流线程
     */
    void stop();

    /**
     * @brief 推理模型池初始化
     */
    void init_model_pool(ModelType model_type, const std::string& model_path, int pool_size);

private:
    /**
     * @brief 编码循环线程函数，处理队列中的帧并推流
     */
    void encoding_loop();

    /**
     * @brief 编码循环线程函数，处理队列中的帧并推流
     */
    void reading_loop();
    
    /**
     * @brief 初始化FFmpeg相关组件
     * @return 初始化成功返回true，否则返回false
     */
    bool init_ffmpeg();
    
    /**
     * @brief 清理FFmpeg相关资源
     */
    void cleanup();
    
    /**
     * @brief 编码并发送帧数据
     * @param frame 待编码的AVFrame
     * @return 编码发送成功返回true，否则返回false
     */
    bool encode_and_send_frame(const AVFrame* frame);

    
private:
    struct AscendingComparator {
        bool operator()(AVFrame* a, AVFrame* b) {
            return a->pts > b->pts;  // priority_queue是最大堆,返回true会使a排在b后面，这里实现升序
        }
    };

    std::string rtmp_url_;
    int width_;
    int height_;
    int fps_;
    int bitrate_;
    int thread_count_ {0};
    CameraCapture cam_;
    tdpool::ThreadPool pool_;

    std::atomic<bool> running_{false};
    std::thread encoding_thread_;

    ThreadSafeQueue<ModelPtr> model_pool_;
    
    // 帧输入队列
    ThreadSafeQueue<AVFrame*,AscendingComparator> input_queue_;
    ThreadSafeQueue<AVFrame*,AscendingComparator> output_queue_;
    
    // FFmpeg 上下文
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    AVStream* video_stream_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    AVFrame* sws_frame_ = nullptr;
    int64_t pts_ = 0;
};