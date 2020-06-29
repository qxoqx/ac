//
// Created by srt on 2020/6/23.
//

#include <card.h>

#include "card.h"

const std::string &card::getCardNo() const {
    return cardNo;
}

void card::setCardNo(const std::string &cardNo) {
    card::cardNo = cardNo;
}

const std::string &card::getName() const {
    return name;
}

void card::setName(const std::string &name) {
    card::name = name;
}

long card::getCardType() const {
    return cardType;
}

void card::setCardType(long cardType) {
    card::cardType = cardType;
}

const tm &card::getBeginTime() const {
    return beginTime;
}

void card::setBeginTime(const tm &beginTime) {
    card::beginTime = beginTime;
}

const tm &card::getEndTime() const {
    return endTime;
}

void card::setEndTime(const tm &endTime) {
    card::endTime = endTime;
}

long card::getEmployeeId() const {
    return employeeID;
}

void card::setEmployeeId(long employeeId) {
    employeeID = employeeId;
}

std::string card::String() {
    char buff[100];
    snprintf(buff, sizeof(buff), "cardNo: %s\t employeeID: %ld\t name: %s", this->cardNo.c_str(), this->employeeID, this->name.c_str());
    return std::string(buff);
}

const std::string &card::getPassword() const {
    return password;
}

void card::setPassword(const std::string &password) {
    card::password = password;
}


