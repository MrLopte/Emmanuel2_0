#ifndef __DHT22_H__
#define __DHT22_H__

#include <cmath>
#include <stdint.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_err.h>
#include <rom/ets_sys.h>
#include <driver/gpio.h>

//Size of response 30ms
#define DATA_LENGTH 4000

//Always 40 bit
#define DATA_PARSED 40

#define FILTER_NUM_DATA 3

class DHT22{

public:

    DHT22(gpio_num_t gpio);
    
    uint32_t waitTimeUpdate;

    void init();

    void updateData();

    //Should be >2 seconds
    //void setSleepTime(int seconds);

    //void setKeepRunning(bool keepReading);

    void toogleDHT22Logs(bool showLogs);

    void DHT22log(const char * string);

    void updateDebugMode();

    int64_t getLastUpdateTime();

    float humidity;
    float temperature;

private:

    uint8_t numBits1Microseconds = 60;
    uint8_t numBits0Microseconds = 20;
    uint8_t num0BeforeSignalMicroseconds = 50;

    int8_t humidityHex;
    int8_t temperatureHex;
    int8_t humidityDecHex;
    int8_t temperatureDecHex;
    
    int8_t checksumHex;

    bool keepReadingPriv;
    bool showLogsPriv;
    int64_t espTimelastUpdatePriv;
    gpio_num_t DHT_GPIOPriv;

    int bitsResponse[DATA_LENGTH]={};
    int bitsResponseparsed[DATA_PARSED]={};
    int bitsResponseparsedOrdered[DATA_PARSED]={};

    bool validateChecksum();

    void sendInitTransmission();

    void listenResponse();

    void parseResponsetoHex();

    void parseDataToFLoat();

    bool checkMinTime();

    bool checkReceivedData();

    void printRawResponse();

    void printHexResponse();

    void printDataNotOrdered();

    void printDataOrdered();

};




#endif //__DHT22_H__