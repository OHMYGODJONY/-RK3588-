#include "yolov5model.h"
#include <iostream>

static unsigned char* load_model(const char* model_path, int* model_len)
{
    FILE *fp = fopen(model_path,"rb");
    if(fp == nullptr)
    {
        std::cerr << "open model faile!" << std::endl;
        return NULL;
    }
    fseek(fp,0,SEEK_END);
    int model_size = ftell(fp);
    unsigned char *model = (unsigned char *)malloc(model_size);
    fseek(fp,0,SEEK_SET);
    if(model_size != fread(model,1,model_size,fp))
    {
        std::cerr << "read model faile!" << std::endl;
        return NULL;
    }
    *model_len = model_size;
    if(fp)
    {
        fclose(fp);
    }
    return model;
}

static void dump_tensor_attr_info(rknn_tensor_attr *attr)
{
    std::cout << "index= " << attr->index << std::endl;
    std::cout << "name= " << attr->name << std::endl;
    std::cout << "n_dims= " << attr->n_dims << std::endl;
    std::cout << "dims= " ;
    for(int i = 0; i < attr->n_dims; i++)
    {
        std::cout << attr->dims[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "n_elems= " << attr->n_elems << std::endl;
    std::cout << "size= " << attr->size << std::endl;
    std::cout << "fmt= " << get_format_string(attr->fmt) << std::endl;
    std::cout << "type= " << get_type_string(attr->type) << std::endl;
    std::cout << "qnt_type= " << get_qnt_type_string(attr->qnt_type) << std::endl;
    std::cout << "zp= " << attr->zp << std::endl;
    std::cout << "scale= " << attr->scale << std::endl;
}

Yolov5Model::Yolov5Model()
{
    ctx = 0;
    model_len = 0;
    channel = 3;
    width   = 0;
    height  = 0;
    nms_threshold = NMS_THRESH;
    box_conf_threshold = BOX_THRESH; 
}

Yolov5Model::~Yolov5Model()
{
    if(ctx > 0)
    {
        rknn_destroy(ctx);
    }
}

bool Yolov5Model::loadmodel(const char *model_path)
{
    std::cout << "load model ..." << std::endl;
    auto model = load_model(model_path, &model_len);
    int ret = rknn_init(&ctx, model, model_len,0,NULL);
    if(ret < 0)
    {
        std::cerr << "rknn init fail!" << std::endl;
        return false;
    }
    free(model);
    
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if(ret != RKNN_SUCC)
    {
        std::cerr << "rknn query num fail!" << std::endl;
        return false;
    }
    std::cout << "model input num: " << io_num.n_input << " ,output num: " << io_num.n_output << std::endl;

    std::cout << "model input attr: " << std::endl;
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));

    for(int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if(ret != RKNN_SUCC)
        {
            std::cerr << "rknn query input_attr fail!" << std::endl;
            return false;
        }
        input_atts.push_back(input_attrs[i]);
        dump_tensor_attr_info(&(input_attrs[i]));
    }

    std::cout << "model output attr: " << std::endl;
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));

    for(int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if(ret != RKNN_SUCC)
        {
            std::cerr << "rknn query output_attr fail!" << std::endl;
            return false;
        }
        output_atts.push_back(output_attrs[i]);
        dump_tensor_attr_info(&(output_attrs[i]));
    }

    if(input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        std::cout << "model input fmt RKNN_TENSOR_NCHW" << std::endl;
        channel = input_attrs[0].dims[1];
        height = input_attrs[0].dims[2];
        width = input_attrs[0].dims[3];
    }

    if(input_attrs[0].fmt == RKNN_TENSOR_NHWC)
    {
        std::cout << "model input fmt RKNN_TENSOR_NHWC" << std::endl;
        height = input_attrs[0].dims[1];
        width = input_attrs[0].dims[2];
        channel = input_attrs[0].dims[3];
    }
    std::cout << "input image height: " << height << " width: " << width << " channels: " << channel << std::endl;
    return true;
}

bool Yolov5Model::run(cv::Mat &img)
{
    cv::Mat test_img = img;
    rknn_input inputs[io_num.n_input];
    memset(inputs, 0, sizeof(inputs));
    for(int i = 0; i < io_num.n_input; i++)
    {
        inputs[i].index = i;
        inputs[i].type = RKNN_TENSOR_UINT8;
        inputs[i].size = height * width * channel;
        inputs[i].fmt = input_atts[i].fmt;
        inputs[i].pass_through = 0;
    }

    if(test_img.empty()) 
    {
        std::cerr << "capture.read error!" << std::endl;
    }

    int img_width = test_img.cols;
    int img_height = test_img.rows;

    if (img_width != width || img_height != height) 
    {
        cv::resize(test_img,test_img,cv::Size(width,height));
        inputs[0].buf = (void*)test_img.data;
    } 
    else 
    {
        inputs[0].buf = (void*)test_img.data;
    }
    auto ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    if(ret < 0 )
    {
        std::cerr << "rknn_inputs_set fail!" << std::endl;
        return false;
    }

    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for(int i = 0; i < io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = 0;
    }


    ret = rknn_run(ctx, NULL);  
    if(ret < 0 )
    {
        std::cerr << "rknn_run fail!" << std::endl;
        return false;
    }
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
    if(ret < 0 )
    {
        std::cerr << "rknn_outputs_get fail!" << std::endl;
        return false;
    }

    float scale_w = (float)width / img_width;
    float scale_h = (float)height / img_height;

    detect_result_group_t detect_result_group;
    std::vector<float> out_scales;
    std::vector<int32_t> out_zps;
    for(int i = 0; i < io_num.n_output; i++)
    {
        out_scales.push_back(output_atts[i].scale);
        out_zps.push_back(output_atts[i].zp);
    }

    post_process((int8_t *)outputs[0].buf, (int8_t *)outputs[1].buf, (int8_t *)outputs[2].buf, 
                height, width,box_conf_threshold, nms_threshold, scale_w, scale_h,
                out_zps, out_scales, &detect_result_group);

    char text[256];
    for (int i = 0; i < detect_result_group.count; i++) 
    {
        detect_result_t* det_result = &(detect_result_group.results[i]);

        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;
        cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 0, 0),10);
        sprintf(text, "%s %.1f%%", det_result->name, det_result->prop * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        int x = det_result->box.left;
        int y = det_result->box.top - label_size.height - baseLine;
        if (y < 0) y = 0;
        if (x + label_size.width > test_img.cols) x = test_img.cols - label_size.width;

        cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), cv::Scalar(255, 255, 255), -1);

       cv::putText(img, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    } 
    ret = rknn_outputs_release(ctx, io_num.n_output, outputs);
    if (ret < 0)
    {
        std::cerr << "rknn_outputs_release fail!" << std::endl;
        return false;
    }
    return true;
}