//
// Created by srt on 2020/6/29.
//

#ifndef AC_ENV_H
#define AC_ENV_H



static std::string envAccessAddr = "ENV_ACCESS_ADDR";
static std::string envAccessUsername = "ENV_ACCESS_USERNAME";
static std::string envAccessPassword = "ENV_ACCESS_PASSWORD";
static std::string envCaptureAddr = "ENV_CAPTURE_ADDR";
static std::string envCaptureUsername = "ENV_CAPTURE_USERNAME";
static std::string envCapturePassword = "ENV_CAPTURE_PASSWORD";
static std::string envWebsocketServer = "ENV_WS_SERVER";

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

static const char* getWebsocketServer() {
    return std::getenv(envWebsocketServer.c_str());
}


#endif //AC_ENV_H