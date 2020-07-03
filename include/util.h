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
/*
 * {
 *      "topic": "ReportError",
 *      "error": "", //空字符串为正常,非空为通讯错误先不阻塞
 *      "payload": {
 *          "running_state": 1,
 *          "equipment_state": 2,
 *      }
 * }
 *   //1.垃圾分类设备正常；
   //2.垃圾桶已满；
   //3.其他异常
   RunningState int `json:"running_state"` //判断垃圾分类设备是否故障或垃圾桶已装满
   //bit0=1，驱动0异常；
   //bit1=1，驱动1异常bit0=1，驱动0异常；
   //bitN=1，驱动N异常； 2<=N<=18
   //bit19=1，驱动19异常；
   //bit20=1，急停；
   //bit21=1，其他垃圾光幕异常；
   //bit22=1，易腐垃圾光幕异常；
   //bit23至bit31未定义
   EquipmentState int `json:"equipment_state"` //判断垃圾分类设备故障节点
 *
 */
#define MACHINE_REPORT_SET_MODE "ReportSetMode"

enum PERSON_STATUS {
    PERSON_EXIT = 1,
    PERSON_LOGIN = 2,
    PERSON_DROPPING = 3,
    PERSON_WAITING = 4,
};

enum MACHINE_STATUS {
    MACHINE_NORMAL = 1,
    MACHINE_GARBAGE_FULL = 2,
    MACHINE_ERROR = 3,
};

class util {

};


#endif //AC_UTIL_H
