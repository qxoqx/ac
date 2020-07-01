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

// 工控机ws服务端地址
static std::string envMachineWSServer = "ENV_MACHINE_WS_SERVER";

// 设备号
static std::string envDeviceNo = "ENV_DEVICE_NUMBER";

static const char* getAccessAddr()  {
    return std::getenv(envAccessAddr.c_str());
}

static const char* getAccessUsername() {
    return std::getenv(envAccessUsername.c_str());
}

static const char* getAccessPassword() {
    return std::getenv(envAccessPassword.c_str());
}

static const char* getCaptureAddr()  {
    return std::getenv(envCaptureAddr.c_str());
}

static const char* getCaptureUsername() {
    return std::getenv(envCaptureUsername.c_str());
}

static const char* getCapturePassword() {
    return std::getenv(envCapturePassword.c_str());
}

static const char* getBackendWSServer() {
    return std::getenv(envBackendWSServer.c_str());
}

static const char* getDeviceNo() {
    return std::getenv(envDeviceNo.c_str());
}

static const char* getMachineWSServer() {
    return std::getenv(envMachineWSServer.c_str());
}


#endif //AC_ENV_H