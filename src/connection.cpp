//
// Created by srt on 2020/6/23.
//
#include "HCNetSDK.h"
#include <cstring>

#include "connection.h"
#include "card.h"

#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <fstream>
#include <spdlog/spdlog.h>

bool connection::threaded = false;


connection::connection(const char* srcIp,const char* username,const char* password) {
    this->srcIp = srcIp;
    this->userName = username;
    this->password = password;

//    char cryptoPath[2048] = {0};
//    sprintf(cryptoPath, "./libcrypto.so");
//    bool cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_LIBEAY_PATH, cryptoPath);
//    spdlog::debug("init lib libcrypto.so {}", cfgInit ? "success" : "fail");
//
//    char sslPath[2048] = {0};
//    sprintf(sslPath, "./libssl.so");
//    cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SSLEAY_PATH, sslPath);
//    spdlog::debug("init lib libssl.so {}", cfgInit ? "success" : "fail");
//
//    NET_DVR_LOCAL_SDK_PATH struComPath = {0};
//    sprintf(struComPath.sPath, "./"); //HCNetSDKCom文件夹所在的路径
//    cfgInit = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struComPath);
//    spdlog::debug("init lib HCNetSDKCom {}", cfgInit ? "success" : "fail");



}

connection::~connection() {
    if(this->isLogined) {
        NET_DVR_Logout_V30(this->lUserID);
    }
    NET_DVR_Cleanup();
}

bool connection::isLogin() const {
    return this->isLogined;
}

DWORD connection::getLastError() {
    return NET_DVR_GetLastError();
}

void connection::doConnect() {
    NET_DVR_Init();

    long lUserID;

    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
    struLoginInfo.bUseAsynLogin = false;

    struLoginInfo.wPort = 8000;
    memcpy(struLoginInfo.sDeviceAddress, this->srcIp.c_str(), NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sUserName, this->userName.c_str(), NAME_LEN);
    memcpy(struLoginInfo.sPassword, this->password.c_str(), NAME_LEN);

    lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

    if (lUserID < 0) {
        spdlog::error("Login error: {}", NET_DVR_GetLastError());
        this->isLogined = false;
    } else {
        this->isLogined = true;
        this->lUserID = lUserID;

        this->serialNumber = std::string((char*) &struDeviceInfoV40.struDeviceV30.sSerialNumber);
        this->channelStart = struDeviceInfoV40.struDeviceV30.byStartChan;
        this->channelNums = struDeviceInfoV40.struDeviceV30.byChanNum;

    }
}


bool connection::doCapture(const char* saveDst) const {

    NET_DVR_JPEGPARA strPicPara = {0};
    strPicPara.wPicQuality = 2;
    strPicPara.wPicSize = 0;

    long startChannel = this->channelStart;
    for(int i = 0 ; i < this->channelNums; i++) {
        int iRet;
        std::string fileName = std::string(saveDst) + std::to_string(startChannel) + ".jpeg";
        iRet = NET_DVR_CaptureJPEGPicture(lUserID, startChannel++, &strPicPara, (char*)fileName.c_str());
        if (!iRet) {
            DWORD ret = NET_DVR_GetLastError();
            spdlog::error("NET_DVR_CaptureJPEGPicture error: {}", ret);
            return false;
        }
    }

    return true;
}

std::vector<card> connection::doGetCards() const {
    NET_DVR_CARD_COND cond = {0};
    cond.dwCardNum = 0xffffffff;
    cond.dwSize = sizeof(NET_DVR_CARD_COND);
    LONG ret = NET_DVR_StartRemoteConfig(this->lUserID, NET_DVR_GET_CARD, &cond, cond.dwSize, nullptr, nullptr);
    if (ret < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return std::vector<card>{};
    }
    LONG remoteRet = ret;

    std::vector<card> cardList;

    NET_DVR_CARD_RECORD record = {0};
    record.dwSize = sizeof(NET_DVR_CARD_RECORD);
    ret = 0;
//    ret = NET_DVR_GetNextRemoteConfig(ret, &record, sizeof(NET_DVR_CARD_RECORD));
//    printf("111 NET_DVR_GetNextRemoteConfig %d\n", ret);

    while (true) {
        if (ret == NET_SDK_GET_NEXT_STATUS_SUCCESS) {
            spdlog::debug("cardNo: {} got", record.byCardNo);

            card newCard;
            newCard.setCardNo(std::string((char*)&record.byCardNo));
            newCard.setCardType(record.byCardType);

            std::tm begin{}, end{};
            begin.tm_year = record.struValid.struBeginTime.wYear - 1900;
            begin.tm_mon = record.struValid.struBeginTime.byMonth - 1;
            begin.tm_mday = record.struValid.struBeginTime.byDay;
            begin.tm_hour = record.struValid.struBeginTime.byHour;
            begin.tm_min = record.struValid.struBeginTime.byMinute;
            begin.tm_sec = record.struValid.struBeginTime.bySecond;

            end.tm_year = record.struValid.struEndTime.wYear - 1900;
            end.tm_mon = record.struValid.struEndTime.byMonth - 1;
            end.tm_mday = record.struValid.struEndTime.byDay;
            end.tm_hour = record.struValid.struEndTime.byHour;
            end.tm_min = record.struValid.struEndTime.byMinute;
            end.tm_sec = record.struValid.struEndTime.bySecond;

            newCard.setBeginTime(begin);
            newCard.setEndTime(end);
            newCard.setName(std::string((char*)&record.byName));
            newCard.setEmployeeId(record.dwEmployeeNo);
            cardList.push_back(newCard);


        } else if (ret == NET_SDK_GET_NETX_STATUS_NEED_WAIT) {
            std::this_thread::sleep_for(std::chrono::milliseconds (100));
        } else if (ret == NET_SDK_GET_NEXT_STATUS_FINISH) {
            break;
        }

        record = NET_DVR_CARD_RECORD{0};
        record.dwSize = sizeof(NET_DVR_CARD_RECORD);
        ret = NET_DVR_GetNextRemoteConfig(remoteRet, &record, sizeof(NET_DVR_CARD_RECORD));
        spdlog::debug("NET_DVR_GetNextRemoteConfig {}", ret);
        if (ret < 0) {
            spdlog::error("NET_DVR_GetNextRemoteConfig error: {}", NET_DVR_GetLastError());
            return std::vector<card>{};
        }
    }
    ret = NET_DVR_StopRemoteConfig(remoteRet);
    if (ret < 0) {
        spdlog::error("NET_DVR_StopRemoteConfig error: {}", NET_DVR_GetLastError());
    }
    return cardList;
}

bool connection::doRemoveCard(std::string cardNo) {
    NET_DVR_CARD_COND cond = {0};
    cond.dwCardNum = 1;
    cond.dwSize = sizeof(NET_DVR_CARD_COND);

    LONG ret = NET_DVR_StartRemoteConfig(lUserID, NET_DVR_DEL_CARD, &cond, cond.dwSize, nullptr, nullptr);
    if (ret < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }
    LONG remoteRet = ret;
    NET_DVR_CARD_SEND_DATA card = {0};
    ret = 0;

    while (true) {
        if (ret == NET_SDK_GET_NEXT_STATUS_SUCCESS) {
            spdlog::debug("card deleted");
        } else if (ret == NET_SDK_GET_NETX_STATUS_NEED_WAIT) {
            std::this_thread::sleep_for(std::chrono::milliseconds (100));
        } else if (ret == NET_SDK_GET_NEXT_STATUS_FINISH) {
            break;
        }
        card = NET_DVR_CARD_SEND_DATA{0};
        card.dwSize = sizeof(NET_DVR_CARD_SEND_DATA);
        memcpy(card.byCardNo, cardNo.c_str(), ACS_CARD_NO_LEN);

        NET_DVR_CARD_STATUS status {0};
        status.dwSize = sizeof(NET_DVR_CARD_STATUS);

        DWORD outlen;
        ret = NET_DVR_SendWithRecvRemoteConfig(remoteRet, &card, sizeof(NET_DVR_CARD_SEND_DATA), &status, sizeof(NET_DVR_CARD_STATUS), &outlen);
        if (ret < 0) {
            spdlog::error("NET_DVR_SendWithRecvRemoteConfig error: {}", NET_DVR_GetLastError());
            return false;
        }
    }
    NET_DVR_StopRemoteConfig(remoteRet);

    return true;
}

bool connection::doGetFaces(const std::string &cardNo) const {
    NET_DVR_FACE_COND cond = {0};
    memcpy(cond.byCardNo, cardNo.c_str(), ACS_CARD_NO_LEN);
    cond.dwFaceNum = 0xffffffff;
    cond.dwSize = sizeof(NET_DVR_FACE_COND);
    LONG ret = NET_DVR_StartRemoteConfig(lUserID, NET_DVR_GET_FACE, &cond, cond.dwSize, nullptr, nullptr);
    if (ret < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }
    LONG remoteRet = ret;

    NET_DVR_FACE_RECORD record = {0};
    record.dwSize = sizeof(NET_DVR_FACE_RECORD);
    ret = 0;
//    ret = NET_DVR_GetNextRemoteConfig(ret, &record, sizeof(NET_DVR_CARD_RECORD));
//    printf("111 NET_DVR_GetNextRemoteConfig %d\n", ret);

    int i = 0;
    while (true) {
        if (ret == NET_SDK_GET_NEXT_STATUS_SUCCESS) {
            spdlog::debug("cardNo: {}'s face got", record.byCardNo);
            std::ofstream ouF;

            char buff[100];
            snprintf(buff, sizeof(buff), "./faces/%s_%d.jpeg",record.byCardNo , i++);


            ouF.open(buff, std::ofstream::binary);
            ouF.write(reinterpret_cast<const char*>(record.pFaceBuffer), record.dwFaceLen);
            ouF.close();

        } else if (ret == NET_SDK_GET_NETX_STATUS_NEED_WAIT) {
            std::this_thread::sleep_for(std::chrono::milliseconds (100));
        } else if (ret == NET_SDK_GET_NEXT_STATUS_FINISH) {
            break;
        }

        record = NET_DVR_FACE_RECORD{0};
        record.dwSize = sizeof(NET_DVR_FACE_RECORD);
        ret = NET_DVR_GetNextRemoteConfig(remoteRet, &record, sizeof(NET_DVR_FACE_RECORD));
        spdlog::debug("NET_DVR_GetNextRemoteConfig {}", ret);
        if (ret < 0) {
            spdlog::error("NET_DVR_GetNextRemoteConfig error: {}", NET_DVR_GetLastError());
            return false;
        }
    }

    return true;
}

bool connection::doSetFace(const std::string &cardNo,const std::string &facePath) {

    std::ifstream is(facePath, std::ifstream::in | std::ios::binary);
    // 2. 计算图片长度
    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);
    // 3. 创建内存缓存区
    char *buffer = new char[length];
    // 4. 读取图片
    is.read(buffer, length);
    is.close();
    auto result = this->doSetFace(cardNo, buffer, length);
    free(buffer);
    return result;
}

bool connection::doRemoveFaces(const std::string &cardNo) {

    NET_DVR_FACE_PARAM_CTRL faceCtrl{0};
    faceCtrl.byMode = 0;
    strcpy((char*)faceCtrl.struProcessMode.struByCard.byCardNo, cardNo.c_str());
    faceCtrl.struProcessMode.struByCard.byEnableCardReader[0] = 1;
    faceCtrl.struProcessMode.struByCard.byFaceID[0] = 1;
    faceCtrl.dwSize = sizeof(NET_DVR_FACE_PARAM_CTRL);
    LONG ret = NET_DVR_RemoteControl(this->lUserID, NET_DVR_DEL_FACE_PARAM_CFG, &faceCtrl, sizeof(NET_DVR_FACE_PARAM_CTRL));
    if (ret == 0) {
        spdlog::error("NET_DVR_RemoteControl error: {}", NET_DVR_GetLastError());
        return false;
    }

    return true;
}

bool connection::doSetCard(const card &newcard) {
    NET_DVR_CARD_COND cond = {0};
    cond.dwCardNum = 1;
    cond.dwSize = sizeof(cond);

    LONG remoteConfigReturn = NET_DVR_StartRemoteConfig(this->lUserID, NET_DVR_SET_CARD, &cond,
            sizeof(NET_DVR_CARD_COND), nullptr,nullptr);
    if (remoteConfigReturn < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }

    NET_DVR_CARD_RECORD cardRecord = {0};

    if (newcard.getCardNo().size() > ACS_CARD_NO_LEN || newcard.getPassword().size() > CARD_PASSWORD_LEN || newcard.getName().size() > 32) {
        spdlog::error("wrong format card");
        return false;
    }

    memcpy(cardRecord.byCardNo, newcard.getCardNo().c_str(), newcard.getCardNo().size());
    memcpy(cardRecord.byCardPassword, newcard.getPassword().c_str(), newcard.getPassword().size());
    memcpy(cardRecord.byName, newcard.getName().c_str(), newcard.getName().size());
    cardRecord.byLeaderCard = 0;
    cardRecord.byUserType = 0;
    cardRecord.byCardType = newcard.getCardType();
    cardRecord.byDoorRight[0] = 1;
    cardRecord.struValid.byEnable = 1;
    // begin time
    auto b = newcard.getBeginTime();
    cardRecord.struValid.struBeginTime.wYear = b.tm_year + 1900;
    cardRecord.struValid.struBeginTime.byMonth = b.tm_mon + 1;
    cardRecord.struValid.struBeginTime.byDay = b.tm_mday;
    cardRecord.struValid.struBeginTime.byHour = 0;
    cardRecord.struValid.struBeginTime.byMinute = 0;
    cardRecord.struValid.struBeginTime.bySecond = 0;
    // end time
    auto e = newcard.getEndTime();
    cardRecord.struValid.struEndTime.wYear = e.tm_year + 1900;
    cardRecord.struValid.struEndTime.byMonth = e.tm_mon + 1;
    cardRecord.struValid.struEndTime.byDay = e.tm_mday;
    cardRecord.struValid.struEndTime.byHour = 0;
    cardRecord.struValid.struEndTime.byMinute = 0;
    cardRecord.struValid.struEndTime.bySecond = 0;
    cardRecord.wCardRightPlan[0] = 1;
    cardRecord.dwEmployeeNo = newcard.getEmployeeId();
    cardRecord.dwSize = sizeof(NET_DVR_CARD_RECORD);

    NET_DVR_CARD_STATUS out = {0};
    out.dwSize = sizeof(NET_DVR_CARD_STATUS);

    DWORD outlen;
    auto ret = NET_DVR_SendWithRecvRemoteConfig(remoteConfigReturn, &cardRecord, sizeof(NET_DVR_CARD_RECORD), &out, sizeof(NET_DVR_CARD_STATUS), &outlen);
    if (ret < 0) {
        spdlog::error("NET_DVR_SendWithRecvRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }

    if (outlen > 0) {
        if(out.byStatus == 0) {
            spdlog::error("set card failed, err code: {}", out.dwErrorCode);
            return false;
        }
    }

    if(!NET_DVR_StopRemoteConfig(remoteConfigReturn)) {
        spdlog::error("NET_DVR_StopRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }

    return true;
}

bool connection::setAlarm(MSGCallBack callBack) {
    if(connection::threaded) {
        // log
        return false;
    }

    if (NET_DVR_SetDVRMessageCallBack_V50(0, callBack, nullptr) == 0) {
        spdlog::error("NET_DVR_SetDVRMessageCallBack_V50 error: {}", NET_DVR_GetLastError());
        return false;
    }
    NET_DVR_SETUPALARM_PARAM setup = {0};
    setup.byDeployType = 1;
    setup.byFaceAlarmDetection = 2;
    setup.byAlarmInfoType = 1;
    setup.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
    LONG ret = NET_DVR_SetupAlarmChan_V41(lUserID, &setup);
    if (ret < 0) {
        spdlog::error("NET_DVR_SetupAlarmChan_V41 error: {}", NET_DVR_GetLastError());
        return false;
    }
    this->alarmHandler = ret;
    connection::threaded = true;

    return true;
}

bool connection::unsetAlarm() {
    if (NET_DVR_CloseAlarmChan_V30(this->alarmHandler) == 0) {
        spdlog::error("NET_DVR_CloseAlarmChan_V30 error: {}", NET_DVR_GetLastError());
        return false;
    }
    this->threaded = false;
    this->alarmHandler = 0;

    return true;
}

bool connection::getAlarmEvents() {
//    NET_DVR_ACS_EVENT_COND cond = {0};
//    cond.dwSize = sizeof(NET_DVR_ACS_EVENT_COND);
//    LONG ret = NET_DVR_StartRemoteConfig(lUserID, NET_DVR_GET_ACS_EVENT, &cond, sizeof(NET_DVR_ACS_EVENT_COND), nullptr, nullptr);
//    if (ret < 0) {
//        printf("pyd1---NET_DVR_StartRemoteConfig error, %d\n", NET_DVR_GetLastError());
//        return HPR_ERROR;
//    }
//
//    NET_DVR_ACS_EVENT_CFG cfg = {0};
//    cfg.dwSize = sizeof(NET_DVR_ACS_EVENT_CFG);
//
//    LONG cfgRet = NET_DVR_GetNextRemoteConfig(ret, &cfg, sizeof(NET_DVR_ACS_EVENT_CFG));
//    if (cfgRet == NET_SDK_GET_NEXT_STATUS_FINISH || cfgRet == NET_SDK_GET_NEXT_STATUS_FAILED) {
//        return HPR_ERROR;
//    }
//    return HPR_OK;
    return false;
}

int connection::doCapture(char *dst, int length) const {
    NET_DVR_JPEGPARA strPicPara = {0};
    strPicPara.wPicQuality = 2;
    strPicPara.wPicSize = 0;

    long startChannel = this->channelStart;
    spdlog::debug("channel start at {}, channelNums: {}", this->channelStart, this->channelNums);

    int iRet;
    unsigned int picLength = 0;
    iRet = NET_DVR_CaptureJPEGPicture_NEW(lUserID, startChannel, &strPicPara, dst, length, &picLength);
    if (!iRet) {
        DWORD ret = NET_DVR_GetLastError();
        spdlog::error("NET_DVR_CaptureJPEGPicture error: {}", ret);
        return false;
    }
    return picLength;
}

std::shared_ptr<card> connection::doGetCard(const std::string &cardNo) {
    NET_DVR_CARD_COND cond = {0};
    cond.dwCardNum = 1;
    cond.dwSize = sizeof(NET_DVR_CARD_COND);
    LONG ret = NET_DVR_StartRemoteConfig(this->lUserID, NET_DVR_GET_CARD, &cond, sizeof(NET_DVR_CARD_COND), nullptr, nullptr);
    if (ret < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return nullptr;
    }
    auto remoteRet = ret;
    NET_DVR_CARD_SEND_DATA data = {0};
    memcpy(data.byCardNo, cardNo.c_str(), ACS_CARD_NO_LEN);
    data.dwSize = sizeof(NET_DVR_CARD_SEND_DATA);
    NET_DVR_CARD_RECORD record = NET_DVR_CARD_RECORD{0};
    record.dwSize = sizeof(NET_DVR_CARD_RECORD);
    DWORD outDataLen = 0;
    ret = NET_DVR_SendWithRecvRemoteConfig(ret, &data, sizeof(NET_DVR_CARD_SEND_DATA), &record, sizeof(NET_DVR_CARD_RECORD), &outDataLen);
    if (ret < 0) {
        spdlog::error("NET_DVR_SendWithRecvRemoteConfig error: {}", NET_DVR_GetLastError());
        return nullptr;
    }

    ret = NET_DVR_StopRemoteConfig(remoteRet);
    if (ret < 0) {
        spdlog::error("NET_DVR_StopRemoteConfig error: {}", NET_DVR_GetLastError());
    }
    auto newCard = std::make_shared<card>();
    newCard->setCardNo(std::string((char*)&record.byCardNo));
    newCard->setCardType(record.byCardType);

    std::tm begin{}, end{};
    begin.tm_year = record.struValid.struBeginTime.wYear - 1900;
    begin.tm_mon = record.struValid.struBeginTime.byMonth - 1;
    begin.tm_mday = record.struValid.struBeginTime.byDay;
    begin.tm_hour = record.struValid.struBeginTime.byHour;
    begin.tm_min = record.struValid.struBeginTime.byMinute;
    begin.tm_sec = record.struValid.struBeginTime.bySecond;

    end.tm_year = record.struValid.struEndTime.wYear - 1900;
    end.tm_mon = record.struValid.struEndTime.byMonth - 1;
    end.tm_mday = record.struValid.struEndTime.byDay;
    end.tm_hour = record.struValid.struEndTime.byHour;
    end.tm_min = record.struValid.struEndTime.byMinute;
    end.tm_sec = record.struValid.struEndTime.bySecond;

    newCard->setBeginTime(begin);
    newCard->setEndTime(end);
    newCard->setName(std::string((char*)&record.byName));
    newCard->setEmployeeId(record.dwEmployeeNo);

    return newCard;
}

bool connection::doSetFace(const std::string &cardNo, const char *buffer, int length) {
    spdlog::debug("face picture length: {}", length);
    NET_DVR_FACE_COND faceCond = {0};
    faceCond.dwFaceNum = 1;
    faceCond.dwEnableReaderNo = 1;
    memcpy(faceCond.byCardNo, cardNo.c_str(), ACS_CARD_NO_LEN);
    faceCond.dwSize = sizeof(NET_DVR_FACE_COND);

    LONG ret = NET_DVR_StartRemoteConfig(lUserID, NET_DVR_SET_FACE, &faceCond, sizeof(NET_DVR_FACE_COND), nullptr, nullptr);
    if (ret < 0) {
        spdlog::error("NET_DVR_StartRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }

    NET_DVR_FACE_RECORD faceRecord = {0};
    memcpy(faceRecord.byCardNo, cardNo.c_str(), ACS_CARD_NO_LEN);
    faceRecord.dwFaceLen = length;
//    memcpy(faceRecord.pFaceBuffer, buffer, length);
    faceRecord.pFaceBuffer = (BYTE*)buffer;
    faceRecord.dwSize = sizeof(NET_DVR_FACE_RECORD);

    NET_DVR_FACE_STATUS result = {0};
    DWORD resultLen;
    LONG new_ret = NET_DVR_SendWithRecvRemoteConfig(ret, &faceRecord, sizeof(NET_DVR_FACE_RECORD), &result, sizeof(NET_DVR_FACE_STATUS), &resultLen);
    if (new_ret < 0) {
        spdlog::error("NET_DVR_SendWithRecvRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }
    auto returnBool = true;
    spdlog::debug("NET_DVR_SendWithRecvRemoteConfig return: {}", new_ret);
    if (resultLen > 0) {
        if (result.byRecvStatus == 1) {
            spdlog::info("set face succeed");
        } else {
            spdlog::error("set face error, RecvStatus: {}", result.byRecvStatus);
            returnBool = false;

        }
    }
    //人脸读卡器状态,按字节表示,0-失败,1-成功,2-重试或人脸质量差,
//    3-内存已满(人脸数据满),4-已存在该人脸,5-非法人脸 ID,6-算法建模失败,7-未下发卡权限,8-未定义(保留)
//            ,9-人眼间距小距小,10-图片数据长度小于 1KB,11-图片格式不符(png/jpg/bmp),
//            12-图片像素数量超过上限,13-图片像素数量低于下限,14-图片信息校验失败,15-图片解码失败,
//            16-人脸检测失败,17-人脸评分失败

    ret = NET_DVR_StopRemoteConfig(ret);
    if (ret == 0) {
        spdlog::error("NET_DVR_StopRemoteConfig error: {}", NET_DVR_GetLastError());
        return false;
    }

    return returnBool;
}


bool connection::DoClearAll() {
    auto cards = this->doGetCards();

    for (auto &&card : cards) {
        if (this->doRemoveFaces(card.getCardNo())) {
            spdlog::debug("{}'s face deleted", card.getCardNo());
        }
        if (this->doRemoveCard(card.getCardNo())) {
            spdlog::debug("{}'s card deleted", card.getCardNo());
        }
    }
}