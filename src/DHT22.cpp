#include "DHT22.h"

DHT22::DHT22(gpio_num_t gpio){

    DHT_GPIOPriv = gpio;

    //Microseconds signal to "3.3V" to detect a 1
    numBits1Microseconds = 60;

    //Microseconds signal to "3.3V" to detect a 0
    numBits0Microseconds = 20;

    //Microseconds signal to "0V" to start detecting next bit
    num0BeforeSignalMicroseconds = 50;

}

void DHT22::init(){
    //esp_timer_get_time(void);
    gpio_config_t config;
    config.mode = GPIO_MODE_INPUT_OUTPUT;
    config.pin_bit_mask = 1 << DHT_GPIOPriv;
    config.intr_type = GPIO_INTR_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&config);

    humidity = 0.0f;
    temperature = 0.0f;

    sendInitTransmission();
}

void DHT22::updateData(){
    //Check if min time is passed
    if(!checkMinTime()){
        return;
    }

    //Store time update
    espTimelastUpdatePriv = esp_timer_get_time();

    sendInitTransmission();

    listenResponse();

    parseResponsetoHex();

    if(((int)temperatureHex < -30) || ((int)temperature > 60)){
        ESP_LOGE("DHT22", "vvvvvvPosible error de lectura!vvvvvv");
        printRawResponse();
        printDataNotOrdered();
        printDataOrdered();
        ESP_LOGE("DHT22", "^^^^^^Posible error de lectura!^^^^^^");
    }else{
        parseDataToFLoat();
    }
}



bool DHT22::validateChecksum(){
    bool ok = false;

    return ok;
}

void DHT22::sendInitTransmission(){

    for(int r=0; r<DATA_LENGTH; r++){bitsResponse[r] =0;}
    
    gpio_set_direction(DHT_GPIOPriv, GPIO_MODE_OUTPUT);

    //Sends a logic 0 for 20 ms 
    gpio_set_level(DHT_GPIOPriv, 0);
    ets_delay_us(20 * 1000);

    //Sends a logic 0 for 40 microseconds 
    gpio_set_level(DHT_GPIOPriv, 1);
    ets_delay_us(40);

    //Configure gpio to read the response
    gpio_set_direction(DHT_GPIOPriv, GPIO_MODE_INPUT);

    for(int r=0; r<DATA_LENGTH; r++){
        bitsResponse[r] =gpio_get_level(DHT_GPIOPriv);
        ets_delay_us(1);
    }    
}

void DHT22::listenResponse(){

   //while(1){
   //    //espera a que responda
   //    if(gpio_get_level(DHT_GPIOPriv)==1){
   //        ets_delay_us(1);
   //    }else{
   //        break;
   //    }
   //    
   //}
    //for(int r=0; r<DATA_LENGTH; r++){
    //    bitsResponse[r] =gpio_get_level(DHT_GPIOPriv);
    //    ets_delay_us(1);
    //}    
    //return;
}

void DHT22::parseResponsetoHex(){
    int lastbit =  bitsResponse[0];
    int newcount=0;
    int sizeparsed =0;
    int nextBitTx=0;
    
    for(int i=0; i<DATA_LENGTH; i++){
        newcount++;
        if((i>0) & (bitsResponse[i]!=lastbit) ){

            if((lastbit == 0) & (newcount>30) & (newcount<60)){
                //Init tx bit
                nextBitTx = 1;
            }
            if((lastbit == 1) & (newcount>40) & (newcount<90)){
                // 1 detected 
                //if(nextBitTx==1){
                    bitsResponseparsed[sizeparsed] = 1;
                    sizeparsed++;
                //}
                nextBitTx = 0;
            }
            if((lastbit == 1) & (newcount>10) & (newcount<30)){
                //0 detected
                //if(nextBitTx==1){
                    bitsResponseparsed[sizeparsed] = 0;
                    sizeparsed++;
                //}
                nextBitTx = 0;
            }
            newcount=0;
        }
        lastbit = bitsResponse[i];
    }

    // TODO: NO SE POQUE EL PPRIMER BIT NO VALE
    //order bits 
    int baux[40] ={};

    for(int i=0; i<39; i++){
        baux[i] = bitsResponseparsed[i+1];
        bitsResponseparsedOrdered[i] = baux[i];
    }
    bitsResponseparsedOrdered[39] = 1;


    humidityHex = 0x00;
    humidityDecHex = 0x00;
    temperatureHex = 0x00;
    temperatureDecHex = 0x00;

    for(int i=0; i<40; i++){

        if(i<8){
            humidityHex = humidityHex | (bitsResponseparsedOrdered[i] << (7-i));
            //printf("Baux: %d, Humedad paso %d: %d\n",bitsResponseparsed[i], i, (int)humidityHex);
            
        }
        else if ((i>7) && (i<16)){
            humidityDecHex = humidityDecHex | (bitsResponseparsedOrdered[i] << (15-i));
            //printf("Baux: %d, Humdec paso %d: %d\n",bitsResponseparsed[i], i, (int)humidityDecHex);
        }else if((i>15) && (i<24)){
            temperatureHex = temperatureHex | (bitsResponseparsedOrdered[i] << (23-i));
            //printf("Baux: %d, Temperatura paso %d: %d\n",bitsResponseparsed[i], i, (int)temperatureHex);
        }
        else if ((i>23) && (i<32)){
            temperatureDecHex = temperatureDecHex | (bitsResponseparsedOrdered[i] << (31-i));
            //printf("Baux: %d, TemperaturaDec paso %d: %d\n",bitsResponseparsed[i], i, (int)temperatureDecHex);
        } 
        else{
            checksumHex = checksumHex | (bitsResponseparsedOrdered[i] << (39-i));
        }
    }
}

void DHT22::parseDataToFLoat(){
    float humAux = ((int)humidityHex) + ((int) humidityDecHex)/10.0f;
    float tempAux = (int)temperatureHex + ((int) temperatureDecHex)/10.0f;

    humidity = floor(humAux*100)/100;
    temperature = floor(tempAux*100)/100;
    
    //humidity = (int)humidityHex + ((int) humidityDecHex)/10.0f;
    //temperature = (int)temperatureHex + ((int) temperatureDecHex)/10.0f;
}


bool DHT22::checkMinTime(){

    int64_t actualTime =  esp_timer_get_time();
    int64_t timePassed = actualTime - espTimelastUpdatePriv;

    if((int)(timePassed) < 2000000){
        ESP_LOGE("DHT22","Time between update cant be less than 2 seconds, setting 2s");
        return false;
    }

    return true;
}

void DHT22::DHT22log(const char * string){
   
    if(showLogsPriv == true){
        ESP_LOGE("DHT22","%s", string);
    }   
    return;
}

void DHT22::toogleDHT22Logs(bool showLogs){
    showLogsPriv = showLogs;
    return;
}

int64_t DHT22::getLastUpdateTime(){
    return espTimelastUpdatePriv;
}
bool DHT22::checkReceivedData(){
    bool out = false;

    if((humidityHex + humidityDecHex + temperatureHex + temperatureDecHex) == checksumHex){
        out=true;
    }
    return out;
}

void DHT22::printRawResponse(){
    int lastbit =  bitsResponse[0];
    int newcount=0;

    printf("\n***RAW RESPONSE*****************************************\n");
    for(int i=0; i<DATA_LENGTH; i++){
        printf("%d ", bitsResponse[i]);
        newcount++;
        if((i>0) & (bitsResponse[i]!=lastbit)){
            printf("\n");
            printf("Contados %d del tipo %d\n", newcount, lastbit);
            if((lastbit == 0) & (newcount>30) & (newcount<60)){
                  printf("inicio transmision bit\n");
            }
            if((lastbit == 1) & (newcount>40) & (newcount<90)){
                  printf("\tHe visto un 1\n\n");
                  
            }
            if((lastbit == 1) & (newcount>10) & (newcount<30)){
                  printf("\tHe visto un 0\n\n");
                  
            }
            //printf("\n");
            newcount=0;
        }
        lastbit = bitsResponse[i];
    }
    printf("\n********************************************************\n");
}

void DHT22::printHexResponse(){
    int lastbit =  bitsResponse[0];
    int newcount=0;

    
    for(int i=0; i<DATA_LENGTH; i++){
        newcount++;
        if((i>0) & (bitsResponse[i]!=lastbit)){

            printf("Contados %d del tipo %d\n", newcount, lastbit);
            if((lastbit == 0) & (newcount>30) & (newcount<60)){
                  printf("inicio transmision bit\n");
            }
            if((lastbit == 1) & (newcount>40) & (newcount<90)){
                  printf("\tHe visto un 1\n\n");
                  
            }
            if((lastbit == 1) & (newcount>10) & (newcount<30)){
                  printf("\tHe visto un 0\n\n");
                  
            }
            //printf("\n");
            newcount=0;
        }
        lastbit = bitsResponse[i];
    }

    printf("Datos leidos raw\n");
    printf("\t- Humidity\n");
    printf("\t- Humidity decimal\n");
    printf("\t- Temperature\n");
    printf("\t- Temperature decimal\n");
    printf("\t- Checksum\n\n");
    
}

void DHT22::printDataNotOrdered(){
    printf("Datos leidos NO ordenados\n");
    for(int i=0; i<40; i++){
        printf("%d ",bitsResponseparsed[i]);
        if(((i+1)%4)==0 && i!=0){
            printf(" ");
        }
        if(((i+1)%8)==0 && i!=0){
            printf(" ");
        }
    }printf("\n");
}

void DHT22::printDataOrdered(){
    
    printf("Datos leidos ordenados\n");
    for(int i=0; i<40; i++){
        printf("%d ",bitsResponseparsedOrdered[i]);
        if(((i+1)%4)==0 && i!=0){
            printf(" ");
        }
        if(((i+1)%8)==0 && i!=0){
            printf(" ");
        }
    }printf("\n");
}

void DHT22::updateDebugMode(){

    toogleDHT22Logs(true);

    DHT22log("----------------MODO UPDATE DEBUG----------------\n");

    DHT22log("Comprobando si se puede actualizar valores\n\n");
    if(!checkMinTime()){
         printf("Han pasado %d microsegundos. Muy pronto, te toca esperar campeon\n\n", (int)(esp_timer_get_time()-espTimelastUpdatePriv));
        DHT22log("----------------MODO UPDATE DEBUG----------------\n\n");
        return;
    }
    printf("Han pasado %d microsegundos, se procede a actualizar.\n\n", (int)(esp_timer_get_time()-espTimelastUpdatePriv));

    printf("Tiempo anterior de actualización: %d microsegundos\n", (int)espTimelastUpdatePriv);
    espTimelastUpdatePriv = esp_timer_get_time();
    printf("Tiempo ahora de actualización: %d microsegundos\n\n", (int)espTimelastUpdatePriv);

    DHT22log("Transmission and listen start\n");

    //
    //Sendingn init transmision
    //
        gpio_set_direction(DHT_GPIOPriv, GPIO_MODE_OUTPUT);

        //Sends a logic 0 for 20 ms 
        gpio_set_level(DHT_GPIOPriv, 0);
        ets_delay_us(20 * 1000);

        //Sends a logic 0 for 40 microseconds 
        gpio_set_level(DHT_GPIOPriv, 1);
        ets_delay_us(40);

        //Configure gpio to read the response
        gpio_set_direction(DHT_GPIOPriv, GPIO_MODE_INPUT);

    //
    //listenResponse();
    //
        for(int r=0; r<DATA_LENGTH; r++){
            bitsResponse[r] =gpio_get_level(DHT_GPIOPriv);
            ets_delay_us(1);
        }    
        
    DHT22log("Transmission and listen response end\n\n");
    
    DHT22log("Printing raw response\n");
    printRawResponse();
    DHT22log("Parsing response to float start\n\n");

    DHT22log("Parsing response to HEX start\n");
    //
    //parseResponsetoHex();
    //
        int lastbit =  bitsResponse[0];
        int newcount=0;
        int sizeparsed =0;
        
        for(int i=0; i<DATA_LENGTH; i++){
            newcount++;
            if((i>0) & (bitsResponse[i]!=lastbit) ){

                if((lastbit == 0) & (newcount>30) & (newcount<60)){
                    //Init tx bit
                }
                if((lastbit == 1) & (newcount>40) & (newcount<90)){
                    // 1 detected 
                    bitsResponseparsed[sizeparsed] = 1;
                    sizeparsed++;
                }
                if((lastbit == 1) & (newcount>10) & (newcount<30)){
                    //0 detected
                    bitsResponseparsed[sizeparsed] = 0;
                    sizeparsed++;
                }
                newcount=0;
            }
            lastbit = bitsResponse[i];
        }

        printf("Datos leidos sin ordenar\n");
        for(int i=0; i<40; i++){
            printf("%d ",bitsResponseparsed[i]);
            if(((i+1)%4)==0 && i!=0){
                printf(" ");
            }
            if(((i+1)%8)==0 && i!=0){
                printf(" ");
            }
        }printf("\n");

        //NO SE POQUE EL PPRIMER BIT NO VALE
        //order bits 
        int baux[40] ={};

        for(int i=0; i<39; i++){
            baux[i] = bitsResponseparsed[i+1];
            bitsResponseparsedOrdered[i] = baux[i];
        }
        bitsResponseparsedOrdered[39] = 1;

        printf("Datos leidos ordenados\n");
        for(int i=0; i<40; i++){
            printf("%d ",bitsResponseparsedOrdered[i]);
            if(((i+1)%4)==0 && i!=0){
                printf(" ");
            }
            if(((i+1)%8)==0 && i!=0){
                printf(" ");
            }
        }printf("\n");

        humidityHex = 0x00;
        humidityDecHex = 0x00;
        temperatureHex = 0x00;
        temperatureDecHex = 0x00;

        for(int i=0; i<40; i++){

            if(i<8){
                humidityHex = humidityHex | (bitsResponseparsedOrdered[i] << (7-i));
                //printf("Baux: %d, Humedad paso %d: %d\n",bitsResponseparsed[i], i, (int)humidityHex);
                
            }
            else if ((i>7) && (i<16)){
                humidityDecHex = humidityDecHex | (bitsResponseparsedOrdered[i] << (15-i));
                //printf("Baux: %d, Humdec paso %d: %d\n",bitsResponseparsed[i], i, (int)humidityDecHex);
            }else if((i>15) && (i<24)){
                temperatureHex = temperatureHex | (bitsResponseparsedOrdered[i] << (23-i));
                //printf("Baux: %d, Temperatura paso %d: %d\n",bitsResponseparsed[i], i, (int)temperatureHex);
            }
            else if ((i>23) && (i<32)){
                temperatureDecHex = temperatureDecHex | (bitsResponseparsedOrdered[i] << (31-i));
                //printf("Baux: %d, TemperaturaDec paso %d: %d\n",bitsResponseparsed[i], i, (int)temperatureDecHex);
            } 
            else{
                checksumHex = checksumHex | (bitsResponseparsedOrdered[i] << (39-i));
            }
        }
        
        printf("\n");printf("\n");

    DHT22log("Checking checksum data start\n");
        if(checkReceivedData()){
            ESP_LOGE("DHT22_checkData", "Datos Ok");
        }
        else{
            ESP_LOGE("DHT22_checkData", "Datos erroneos");
        }
        
        bool out = false;

        printf("HumHex: \t%x\n", humidityHex);
        printf("HumHex_dec: \t%x\n", humidityDecHex);
        printf("TempHex: \t%x\n", temperatureHex);
        printf("TempHex_dec: \t%x\n", temperatureDecHex);
        printf("Checksum: \t%x\n\n", checksumHex);
        printf("Sumando: %x + %x + %x + %x - %x\n", humidityHex, humidityDecHex, temperatureHex, temperatureDecHex, checksumHex);
        printf("Suma: %x\n", (humidityHex + humidityDecHex + temperatureHex + temperatureDecHex - checksumHex));

        if((humidityHex + humidityDecHex + temperatureHex + temperatureDecHex) == checksumHex){
            out=true;
        }
    DHT22log("Checking checksum data end\n\n");

    printf("Prueba de salida parseada:\n");
    printf("Temperatura: %d.%d\n", ((int)temperatureHex), ((int)temperatureDecHex));
    printf("Humedad: %d.%d\n", ((int)humidityHex), ((int)humidityDecHex));
    printf("\n");

    DHT22log("Parsing response to HEX end\n\n");

    DHT22log("Parsing response to float start\n");
    parseDataToFLoat();
    DHT22log("Parsing response to float end\n\n");

    //DHT22log("Printing HEX response start\n");
    //printHexResponse();
    //DHT22log("Parsing HEX response end\n\n");

    DHT22log("----------------MODO UPDATE DEBUG----------------\n\n");
    toogleDHT22Logs(false);
}



//Disabled, should be in parent
/*//Should be >2 seconds
void DHT22::setSleepTime(int seconds){

}

void DHT22::setKeepRunning(bool keepReading){
    keepReadingPriv = keepReading;
    return;
}
*/