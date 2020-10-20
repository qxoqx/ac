//
// Created by srt on 2020/6/29.
//

#ifndef AC_ENV_H
#define AC_ENV_H


// 门禁机信息
static std::string envAccessAddr = "ENV_ACCESS_ADDR";
static std::string envAccessUsername = "ENV_ACCESS_USERNAME";
static std::string envAccessPassword = "ENV_ACCESS_PASSWORD";

// 设备号
static std::string envDeviceNo = "ENV_DEVICE_NUMBER";

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

static const char* getDeviceNo() {
    return getEnvSafe(envDeviceNo);
}

static const char* getMachineHTTPServer() {
    return getEnvSafe(envMachineHTTPServer);
}

#endif //AC_ENV_H