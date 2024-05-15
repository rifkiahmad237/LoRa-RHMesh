#pragma once
#ifndef DATAHANDLING_H
#define DATAHANDLING_H
#include <Arduino.h>
#include <functional>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <sys/time.h>
#include "time.h"
#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <esp_task_wdt.h>
#define GateWay Serial2
#define RF95_FREQ 915.0
#define WDT_TIMEOUT 15

#if defined(RFM95_CS) && defined(RFM95_RST) && defined(RFM95_INT)
#endif

const uint8_t selfAddres = 255;
const uint8_t node1Addres = 1;
const uint8_t node2Addres = 2;
// radio driver & message mesh delivery/receipt manager

typedef enum
{
    start_alarm = 1,
    stop_alarm = 2,
    self_test = 3
} alarm_message;

typedef struct gateway_message
{
    alarm_message alarm;
    unsigned long time_stamp;
} gateway_message;

typedef struct power_message
{
    float voltage;
    float current;
    float power;
} power_message;
typedef struct node1_message
{
    float temp;
    float hum;
    float mot_speed;
} node1_message;

typedef struct node2_message
{
    // bool led;
    unsigned long time_stamp;
    float pga;
} node2_message;

struct timeval;
// client mac address
class DataHandling
{
public:
    static DataHandling *GetInstance();
    DataHandling();
    void sendDataToGateway(const uint8_t *struct_message);
    static void recvFromNode(void *);

private:
    // JsonDocument _jsonDoc;
    void iniatilize();
    void recvDataFromGateWay();
    static unsigned long getTime();
    // static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    // static void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
};
#endif