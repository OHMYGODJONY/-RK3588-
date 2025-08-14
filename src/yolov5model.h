#ifndef YOLOV5MODEL_H
#define YOLOV5MODEL_H

#include "Model.h"
#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>
#include "postprocess.h"
#include "rknn_api.h"


class Yolov5Model: public Model
{
public: 
    Yolov5Model();
    ~Yolov5Model();

    bool loadmodel(const char *model_path) override;
    bool run(cv::Mat &img) override;
    std::string get_name() const override {
        return "YOLOV5";
    }

private:
    bool ready_;
    rknn_context ctx;
    int model_len;
    int channel;
    int width;
    int height;
    float nms_threshold;
    float box_conf_threshold; 
    rknn_input_output_num io_num;
    std::vector<rknn_tensor_attr> input_atts;
    std::vector<rknn_tensor_attr> output_atts;
};

#endif