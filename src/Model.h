#ifndef MODEL_H
#define MODEL_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

class Model {
public:
    // 虚析构函数，确保子类析构正常调用
    virtual ~Model() = default;

    // 加载模型
    // 返回值：true=加载成功，false=加载失败
    virtual bool loadmodel(const char *model_path) = 0;

    // 运行模型推理（纯虚函数，子类必须实现）
    // 输入：待处理的图像帧
    // 输出：处理后的图像帧（原图操作）
    // 返回值：true=推理成功，false=推理失败
    virtual bool run(cv::Mat& input) = 0;

    // 获取模型名称/类型，方便调试和日志
    virtual std::string get_name() const = 0;
};

// 定义智能指针，方便管理模型实例
using ModelPtr = std::shared_ptr<Model>;

#endif // MODEL_H