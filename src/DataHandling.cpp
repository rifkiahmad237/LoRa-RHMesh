#include "DataHandling.h"
#define RX2 16
#define TX2 17

SemaphoreHandle_t gateway_mutex;
const char *alarm_command;
const char *alarmCommand[3] = {"START_ALARM", "STOP_ALARM", "TEST_ALARM"};
gateway_message gatewayRecvMessage;
gateway_message gatewaySendMessage;
extern power_message powerMessage;
node1_message node1MessageRx;
node2_message node2MessageRx;
timeval tv;
RH_RF95 RFM95Modem_(RFM95_CS, RFM95_INT);
RHMesh RHMeshManager(RFM95Modem_, selfAddres);
DataHandling *DataHandling::GetInstance()
{
    static DataHandling instance;
    return &instance;
}

DataHandling::DataHandling()
{
    iniatilize();
}

void DataHandling::sendDataToGateway(const uint8_t *struct_message)
{
    if (xSemaphoreTake(gateway_mutex, portMAX_DELAY) == pdTRUE)
    {
        StaticJsonDocument<200> _jsonDocTx;
        JsonArray _jsonArrayTx;
        // Serial.println("\nMasuk SendDatatoGateway");
        // _jsonArray = _jsonDoc.add<JsonArray>();
        // JsonObject client1 = _jsonDoc["client"].to<JsonObject>();
        if (memcmp(&node1MessageRx, struct_message, sizeof(node1MessageRx)) == 0)
        {
            _jsonArrayTx = _jsonDocTx.createNestedArray("node1");
            _jsonArrayTx.add(node1MessageRx.temp);
            _jsonArrayTx.add(node1MessageRx.hum);
            _jsonArrayTx.add(node1MessageRx.mot_speed);
            Serial.printf("\nTemp: %f\n", node1MessageRx.temp);
        }

        else if (memcmp(&node2MessageRx, struct_message, sizeof(node2MessageRx)) == 0)
        {
            _jsonArrayTx = _jsonDocTx.createNestedArray("node2");
            // _jsonArrayTx.add(node2MessageRx.led);
            _jsonArrayTx.add(node2MessageRx.time_stamp);
            _jsonArrayTx.add(node2MessageRx.pga);
            // _jsonArrayTx.add(node2MessageRx.angle_x);
            // _jsonArrayTx.add(node2MessageRx.angle_y);
            // _jsonArrayTx.add(node2MessageRx.angle_z);
        }
        else if (memcmp(&powerMessage, struct_message, sizeof(powerMessage)) == 0)
        {
            _jsonArrayTx = _jsonDocTx.createNestedArray("power");
            _jsonArrayTx.add(powerMessage.voltage);
            _jsonArrayTx.add(powerMessage.current);
            _jsonArrayTx.add(powerMessage.power);
        }

        // serializeJson(_jsonArray, GateWay);
        serializeJson(_jsonDocTx, GateWay);
        xSemaphoreGive(gateway_mutex);
    }
}

void DataHandling::recvDataFromGateWay()
{
    // if (xSemaphoreTakeFromISR(gateway_mutex, NULL) == pdTRUE)
    if (xSemaphoreTake(gateway_mutex, portMAX_DELAY) == pdTRUE)
    {
        StaticJsonDocument<200> _jsonDocRx;
        JsonArray _jsonArrayRx;
        Serial.println("\nMasuk recvDataFromGateway");
        DeserializationError err = deserializeJson(_jsonDocRx, GateWay);
        if (err == DeserializationError::Ok)
        {
            if (_jsonDocRx.containsKey("sync_time"))
            {
                _jsonArrayRx = _jsonDocRx["sync_time"].as<JsonArray>();
                unsigned long timeStamp = _jsonArrayRx[0].as<unsigned long>();
                Serial.print("\n===========================================\n");
                Serial.printf("Epoch Time from Gateway: %lu\n", timeStamp);
                Serial.printf("Time Before Set: %lu\n", (unsigned long)tv.tv_sec);
                unsigned long timeNow = DataHandling::getTime() - timeStamp;
                if (tv.tv_sec == 0 || abs((int)timeNow) > 60)
                {
                    tv.tv_sec = _jsonArrayRx[0].as<unsigned long>();
                    settimeofday(&tv, NULL);
                    Serial.println("\n==== Time Set ====\n");
                }
                Serial.printf("Time Now: %lu \n", DataHandling::getTime());
                Serial.printf("Selisih Waktu: %lu \n", abs((int)timeNow));
                Serial.print("===========================================\n");
            }
            else if (_jsonDocRx.containsKey("sirine_command"))
            {
                Serial.println("Masuk Sirine Command");
                _jsonArrayRx = _jsonDocRx["sirine_command"].as<JsonArray>();
                alarm_command = _jsonArrayRx[1];
                if (strcmp(alarm_command, alarmCommand[0]) == 0)
                {
                    gatewaySendMessage.alarm = start_alarm;
                }
                else if (strcmp(alarm_command, alarmCommand[1]) == 0)
                {
                    gatewaySendMessage.alarm = stop_alarm;
                }
                else if (strcmp(alarm_command, alarmCommand[2]) == 0)
                {
                    gatewaySendMessage.alarm = self_test;
                }
                gatewaySendMessage.time_stamp = DataHandling::getTime();
                Serial.printf("ALARM MESSAGE: %d\n\n", gatewaySendMessage.alarm);
                uint8_t err = RHMeshManager.sendto((uint8_t *)&gatewaySendMessage, sizeof(gateway_message), node1Addres);
                if (err == RH_ROUTER_ERROR_NONE)
                {
                    Serial.printf("Succesfull sending command to Node %d\n", node1Addres);
                }
                else
                {
                    Serial.printf("Failed to send command to node %d\n", node1Addres);
                }
            }
        }
        else
        {
            Serial.printf("JSON Parsing Error: %s\n", err.c_str());

            while (GateWay.available() > 0)
            {
                Serial.read();
            }
        }
        // xSemaphoreGiveFromISR(gateway_mutex, NULL);
        xSemaphoreGive(gateway_mutex);
    }
}

void DataHandling::recvFromNode(void *param)
{
    uint8_t _msgRcvBuf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t _msgRcvBufLen = sizeof(_msgRcvBuf);
    uint8_t nodeAddr;
    uint8_t node1MsgBuffLen = sizeof(node1MessageRx);
    uint8_t node2MsgBuffLen = sizeof(node2MessageRx);
    // const char *recvMsg[1] = {"OK"};
    std::string _msgAck = String("OK").c_str();
    while (true)
    {

        Serial.println("Menunggu data dari node " + (String)node1Addres + " & " + (String)node2Addres);
        if (RHMeshManager.recvfromAckTimeout(_msgRcvBuf, &_msgRcvBufLen, 1000, &nodeAddr))

        {
            Serial.println("Data Masuk");
            if (nodeAddr == node1Addres)
            {
                Serial.println("Data From Node 1");
                mempcpy(&node1MessageRx, _msgRcvBuf, node1MsgBuffLen);
                // Serial.printf("\n===============\nTemperature: %f\n", node1MessageRx.temp);
                // Serial.printf("Humidity: %f\n", node1MessageRx.hum);
                // Serial.printf("Motor State: %d\n===============\n", node1MessageRx.mot_speed);
                Serial.print("Temp: ");
                Serial.println(node1MessageRx.temp);
                Serial.print("Hum: ");
                Serial.println(node1MessageRx.hum);
                Serial.print("Mot_Speed: ");
                Serial.println(node1MessageRx.mot_speed);
                DataHandling::GetInstance()->sendDataToGateway((const uint8_t *)&node1MessageRx);
            }
            else if (nodeAddr == node2Addres)
            {
                Serial.println("Data From Node 2");
                mempcpy(&node2MessageRx, _msgRcvBuf, node2MsgBuffLen);
                Serial.print("time_stamp: ");
                Serial.println(node2MessageRx.time_stamp);
                Serial.print("PGA: ");
                Serial.println(node2MessageRx.pga, 6);
                DataHandling::GetInstance()->sendDataToGateway((const uint8_t *)&node2MessageRx);
            }
            uint8_t err = RHMeshManager.sendtoWait(reinterpret_cast<uint8_t *>(&_msgAck), _msgAck.size(), nodeAddr);
            if (err == RH_ROUTER_ERROR_NONE)
            {
                Serial.println("Succesfull sending Ack to Node " + (String)nodeAddr);
            }
            else
            {
                Serial.println("Failed to send reply....");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void DataHandling::iniatilize()
{
    Serial.begin(115200);
    GateWay.begin(9600, SERIAL_8N1, RX2, TX2);
    GateWay.setRxFIFOFull(120);
    GateWay.onReceive(std::bind(&DataHandling::recvDataFromGateWay, this));
    if (!RHMeshManager.init())
    {
        Serial.println("Setup Failed");
    }
    RFM95Modem_.setTxPower(23, false);
    RFM95Modem_.setFrequency(RF95_FREQ);
    RFM95Modem_.setCADTimeout(500);
    gateway_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(recvFromNode, "recvFromNode", 4096, NULL, 1, NULL, 1);
    Serial.printf("Endnode Lora with Addres %d Initialized \n", selfAddres);
}

unsigned long DataHandling::getTime()
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return (0);
    }
    time(&now);
    return now;
}

// void DataHandling::onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
// {
//     if (memcmp(mac, client1Addr, sizeof(client1Addr)) == 0)
//     {
//         memcpy(&node1MessageRx, incomingData, sizeof(node1MessageRx));
//         // GateWay.write((uint8_t *)&node1MessageRx, sizeof(node1MessageRx));
//         Serial.printf("Temperature: %f\n", node1MessageRx.temp);
//         Serial.printf("Humidty: %f\n", node1MessageRx.hum);
//         Serial.printf("Motor Speed: %d\n", node1MessageRx.mot_speed);
//         DataHandling::GetInstance()->sendDataToGateway((const uint8_t *)&node1MessageRx);
//     }
//     else if (memcmp(mac, client2Addr, sizeof(client2Addr)) == 0)
//     {
//         memcpy(&node2MessageRx, incomingData, sizeof(node2MessageRx));
//         // GateWay.write((uint8_t *)&node1MessageRx, sizeof(client1_message));
//         Serial.print("LED: ");
//         Serial.println(node2MessageRx.led);
//         Serial.printf("Angle X: %f\n", node2MessageRx.angle_x);
//         Serial.printf("Angle Y: %f\n", node2MessageRx.angle_y);
//         Serial.printf("Angle Z: %f\n", node2MessageRx.angle_z);
//         DataHandling::GetInstance()->sendDataToGateway((const uint8_t *)&node2MessageRx);
//     }
// }

// void DataHandling::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//     Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void DataHandling::iniatilize()
// {
//     GateWay.begin(9600, SERIAL_8N1, RX2, TX2);
//     GateWay.setRxFIFOFull(120);
//     GateWay.onReceive(std::bind(&DataHandling::recvDataFromGateWay, this));
//     // Serial.print("Menyiapkan ESP-NOW ");
//     // while (esp_now_init() != ESP_OK)
//     // {
//     //     Serial.print(".");
//     //     delay(100);
//     // }

//     // esp_now_register_send_cb(DataHandling::onDataSent);
//     // memcpy(client1.peer_addr, client1Addr, 6);
//     // client1.channel = 0;
//     // client1.encrypt = false;

//     // memcpy(client2.peer_addr, client2Addr, 6);
//     // client2.channel = 0;
//     // client2.encrypt = false;

//     // while ((esp_now_add_peer(&client1) && esp_now_add_peer(&client2)) != ESP_OK)
//     // {
//     //     Serial.println("\nGagal Menghubungkan Client");
//     // }
//     // Serial.println("\nSUKSES");

//     // esp_now_register_recv_cb(DataHandling::onDataRecv);
// }