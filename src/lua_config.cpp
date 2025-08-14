#include "lua_config.h"
extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}
#include <stdexcept>



// 从Lua配置文件读取摄像头配置列表
std::vector<CameraConfig> read_camera_configs(const std::string& lua_file) {
    std::vector<CameraConfig> configs;

    // 创建Lua状态机
    lua_State *L = luaL_newstate();
    luaopen_base(L);
    luaopen_string(L);
    luaopen_table(L);
    if (!L) {
        throw std::runtime_error("无法创建Lua状态机");
    }

    if(luaL_loadfile(L,lua_file.c_str()))
    {
        std::string err_msg = lua_tostring(L, -1);
        lua_close(L);
        throw std::runtime_error("配置文件加载失败: " + err_msg);
    }

    if(lua_pcall(L,0,0,0))
    {
        std::string err_msg = lua_tostring(L, -1);
        lua_close(L);
        throw std::runtime_error("配置文件执行失败: " + err_msg);
    }

    lua_getglobal(L, "camera_configs");
    if (!lua_istable(L, -1)) {
        lua_close(L);
        throw std::runtime_error("配置文件中未找到camera_configs表");
    }

    lua_pushnil(L);  // 用于遍历的初始key
    while (lua_next(L, -2) != 0) 
    {
        if (!lua_istable(L, -1)) 
        {
            lua_pop(L, 1);  // 弹出无效值
            continue;
        }

        CameraConfig config;

        // 读取device字段
        lua_getfield(L, -1, "device");
        if (lua_isstring(L, -1)) {
            config.device = lua_tostring(L, -1);
        } else {
            throw std::runtime_error("device字段格式错误或不存在");
        }
        lua_pop(L, 1);

        // 读取rtmp_url字段
        lua_getfield(L, -1, "rtmp_url");
        if (lua_isstring(L, -1)) {
            config.rtmp_url = lua_tostring(L, -1);
        } else {
            throw std::runtime_error("rtmp_url字段格式错误或不存在");
        }
        lua_pop(L, 1);

        // 读取width字段
        lua_getfield(L, -1, "width");
        if (lua_isinteger(L, -1)) {
            config.width = lua_tointeger(L, -1);
        } else {
            throw std::runtime_error("width字段格式错误或不存在");
        }
        lua_pop(L, 1);

        // 读取height字段
        lua_getfield(L, -1, "height");
        if (lua_isinteger(L, -1)) {
            config.height = lua_tointeger(L, -1);
        } else {
            throw std::runtime_error("height字段格式错误或不存在");
        }
        lua_pop(L, 1);

        // 读取fps字段
        lua_getfield(L, -1, "fps");
        if (lua_isinteger(L, -1)) {
            config.fps = lua_tointeger(L, -1);
        } else {
            throw std::runtime_error("fps字段格式错误或不存在");
        }
        lua_pop(L, 1);

        configs.push_back(config);
        lua_pop(L, 1);  // 弹出当前配置表

    }
    lua_pop(L, 1);
    lua_close(L);
    return configs;
}

#if 0

int main() {
    try {
        std::vector<CameraConfig> configs = read_camera_configs("Config.lua");
        
        std::cout << "成功读取" << configs.size() << "个摄像头配置:" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "摄像头 " << i << ":" << std::endl;
            std::cout << "  device: " << configs[i].device << std::endl;
            std::cout << "  rtmp_url: " << configs[i].rtmp_url << std::endl;
            std::cout << "  分辨率: " << configs[i].width << "x" << configs[i].height << std::endl;
            std::cout << "  fps: " << configs[i].fps << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
#endif