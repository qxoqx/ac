//
// Created by srt on 2020/6/29.
//

#ifndef AC_ENV_H
#define AC_ENV_H


// 门禁机信息
static std::string envAccessAddr = "ENV_ACCESS_ADDR";
static std::string envAccessUsername = "ENV_ACCESS_USERNAME";
static std::string envAccessPassword = "ENV_ACCESS_PASSWORD";

// 摄像头信息
static std::string envCaptureAddr = "ENV_CAPTURE_ADDR";
static std::string envCaptureUsername = "ENV_CAPTURE_USERNAME";
static std::string envCapturePassword = "ENV_CAPTURE_PASSWORD";

// 后端ws服务端地址
static std::string envBackendWSServer = "ENV_BACKEND_WS_SERVER";
// 设备号
static std::string envDeviceNo = "ENV_DEVICE_NUMBER";

// 工控机ws服务端地址
static std::string envMachineWSServer = "ENV_MACHINE_WS_SERVER";

static std::string envMachineHTTPServer = "ENV_MACHINE_HTTP_SERVER";

static const char* getEnvSafe(const std::string &envName) {
    if (auto p = std::getenv(envName.c_str())) {
        return p;
    } else {
        spdlog::error("no ENV {}", envName);
        return "";
    }
}

static const char* getAccessAddr()  {
    return getEnvSafe(envAccessAddr);
}

static const char* getAccessUsername() {
    return getEnvSafe(envAccessUsername);
}

static const char* getAccessPassword() {
    return getEnvSafe(envAccessPassword);
}

static const char* getCaptureAddr()  {
    return getEnvSafe(envCaptureAddr);
}

static const char* getCaptureUsername() {
    return getEnvSafe(envCaptureUsername);
}

static const char* getCapturePassword() {
    return getEnvSafe(envCapturePassword);
}

static const char* getDeviceNo() {
    return getEnvSafe(envDeviceNo);
}

static const std::string getBackendWSServer() {
    if(const char* env_p = std::getenv(envBackendWSServer.c_str())) {
        std::string backendAddr(env_p);
        if (*(backendAddr.rbegin()) != '/') {
            backendAddr.push_back('/');
        }
        auto deviceNo = getDeviceNo();
        backendAddr.append(deviceNo);
        return backendAddr;
    } else {
        spdlog::error("no ENV {}", envBackendWSServer);
        return "";
    }


}



static const char* getMachineWSServer() {
    return getEnvSafe(envMachineWSServer);
}

static const char* getMachineHTTPServer() {
    return getEnvSafe(envMachineHTTPServer);
}

#endif //AC_ENV_H