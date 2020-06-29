//
// Created by srt on 2020/6/29.
//

#ifndef AC_ENV_H
#define AC_ENV_H

#endif //AC_ENV_H


std::string envAccessAddr = "ENV_ACCESS_ADDR";
std::string envAccessUsername = "ENV_ACCESS_USERNAME";
std::string envAccessPassword = "ENV_ACCESS_PASSWORD";

std::string envCaptureAddr = "ENV_CAPTURE_ADDR";
std::string envCaptureUsername = "ENV_CAPTURE_USERNAME";
std::string envCapturePassword = "ENV_CAPTURE_PASSWORD";

std::string envWebsocketServer = "ENV_WS_SERVER";

const char* getAccessAddr()  {
    return std::getenv(envAccessAddr.c_str());
}

const char* getAccessUsername() {
    return std::getenv(envAccessUsername.c_str());
}

const char* getAccessPassword() {
    return std::getenv(envAccessPassword.c_str());
}

const char* getCaptureAddr()  {
    return std::getenv(envCaptureAddr.c_str());
}

const char* getCaptureUsername() {
    return std::getenv(envCaptureUsername.c_str());
}

const char* getCapturePassword() {
    return std::getenv(envCapturePassword.c_str());
}

const char* getWebsocketServer() {
    return std::getenv(envWebsocketServer.c_str());
}