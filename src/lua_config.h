#pragma once
#include <vector>
#include <string>
#include <iostream>

struct CameraConfig {
    std::string device;
    std::string rtmp_url;
    int width;
    int height;
    int fps;
};

std::vector<CameraConfig> read_camera_configs(const std::string& lua_file);