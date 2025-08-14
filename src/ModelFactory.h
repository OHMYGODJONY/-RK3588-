#ifndef MODEL_FACTORY_H
#define MODEL_FACTORY_H

#include "Model.h"
#include "yolov5model.h"
#include "TestModel.h"
#include <unordered_map>
#include <functional>

enum class ModelType {
    YoloV5,
    Test,
};

class ModelFactory {
public:
    static ModelFactory& get_instance() {
        static ModelFactory instance;
        return instance;
    }

    ModelFactory(const ModelFactory&) = delete;
    ModelFactory& operator=(const ModelFactory&) = delete;

    ModelPtr create_model(ModelType type) {
        auto it = creators_.find(type);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr; 
    }

private:
    ModelFactory() {
        creators_[ModelType::YoloV5] = []() {
            return std::make_shared<Yolov5Model>();
        };
        creators_[ModelType::Test] = []() {
            return std::make_shared<TestModel>();
        };
        // 添加新模型时，注册creators_!!!!!
    }

    std::unordered_map<ModelType, std::function<ModelPtr()>> creators_;
};

#endif // MODEL_FACTORY_H