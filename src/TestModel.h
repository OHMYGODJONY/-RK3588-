#include "Model.h"
#include <thread>

class TestModel: public Model
{
    bool loadmodel(const char *model_path) override {
        std::cout << "model load success" << std::endl;
        return true;
    }

    bool run(cv::Mat& input) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));//模拟任务执行
        cv::cvtColor(input,input,cv::COLOR_RGB2BGR);
        std::cout << "model run success" << std::endl;
        return true;
    }

    std::string get_name() const override {
        return "TestModel";
    }
};