//
// Created by srt on 2020/6/23.
//

#ifndef AC_UTIL_H
#define AC_UTIL_H

#define AS_MACHINE_WS_CLIENT "asMachineClient"
#define AS_BACKEND_WS_CLIENT "asBackendClient"
#define AS_SCREEN_WS_SERVER "asScreenServer"

#define MACHINE_START_DROP "StartDrop"
#define MACHINE_FINISH_DROP "FinishDrop"
#define MACHINE_PHOTO_RESULT "PhotoResult"

#define MACHINE_REPORT_WEIGHT "ReportWeighResult"
#define MACHINE_REPORT_END "ReportEndDeal"
#define MACHINE_REPORT_ERROR "ReportError"
#define MACHINE_REPORT_SET_MODE "ReportSetMode"

enum PERSON_STATUS {
    PERSON_EXIT = 1,
    PERSON_LOGIN = 2,
    PERSON_DROPPING = 3,
    PERSON_WAITING = 4,
};

class util {

};


#endif //AC_UTIL_H
