#include <WiFi.h>
#include <minecraft.h>
//#include "../src/miniz.h"

String username = "nickname";
String server_url = "ip";
int server_port = 25565;

const char* ssid = "ssid";
const char* password =  "pass";

TaskHandle_t listener;
minecraft mc (username, server_url, server_port);
WiFiClient client;

void setup() {
    Serial.begin(115200);
    delay(100);
    WiFi.begin(ssid, password); 
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to the WiFi network");
    Serial.println(WiFi.localIP());

    delay(1000);

    const char* buf;
    buf = server_url.c_str();

    if (!client.connect(buf, server_port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }
    delay(500);
    xTaskCreatePinnedToCore(listener_fun, "listener", 100000, NULL, 2, &listener, 0);
 
    mc.handShake(client, 2);
    Serial.println("[INFO] -> handShake packet sent");
    vTaskDelay(pdMS_TO_TICKS(1000));
    //mc.ping(client, 1);

    mc.loginStart(client);
    Serial.println("[INFO] -> login_start packet sent");
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*mc.clientStatus(client, 0);
    Serial.println("[INFO] -> client_status packet sent");
    delay(2000);*/

    /*mc.writeChat(client, "/login nikk");
    Serial.println("[INFO] -> logging in as nikbot");
    delay(200);*/

    mc.writeChat(client, "test");
    Serial.println("[INFO] -> writing to chat");
    vTaskDelay(pdMS_TO_TICKS(1000));

    disableCore0WDT();
    disableCore1WDT();
}

void loop(){
    vTaskDelay(pdMS_TO_TICKS(50));
    if(mc.teleported){
        mc.setRotation(client, mc.yaw, mc.pitch, mc.onGround);
        Serial.println("[INFO] -> rotation set to: " + String(mc.yaw));
        mc.yaw+=5;
    }
    
    //mc.writeChat(client, "[nik INFO] Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB || buf: " + String(client.available()/1024.0) + "kB");
    Serial.println("[INFO] # buf: " + String(client.available()/1024.0) + "kB");
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

void listener_fun( void * parameter ){
    while(1){
        int pack_length = mc.readVarInt(client);

        if(mc.compression_enabled){
            int data_length = mc.readVarInt(client);

            if(data_length > mc.compression_treshold){
                int len = pack_length - mc.VarIntLength(data_length);
                uint8_t* buf;
                buf = new uint8_t[len];

                client.readBytes(buf, len);

                delete [] buf;

                //Serial.println("[INFO] <- packet received! pack_length: " + String(pack_length) + " data_length: " + String(data_length));
            } else {
                int id = mc.readVarInt(client);

                /*Serial.print("[INFO] <- pack len: " + String(pack_length) + "B id: 0x");
                Serial.println(id, HEX);*/

                switch (id){
                    case 0x49:{
                        mc.health = mc.readFloat(client);
                        mc.food = mc.readVarInt(client);
                        mc.food_sat = mc.readFloat(client);

                        Serial.println("[INFO] <- player stats health: " + String(mc.health)
                                        + " food: " + String(mc.food)
                                        + " food_sat: " + String(mc.food_sat));
                        
                        break;
                    }
                    case 0x36:{
                        mc.x = mc.readDouble(client);
                        mc.y = mc.readDouble(client);
                        mc.z = mc.readDouble(client);
                        mc.yaw = mc.readFloat(client);
                        mc.pitch = mc.readFloat(client);
                        uint8_t flags = client.read();
                        int id = mc.readVarInt(client);
                        mc.teleported = true;

                        Serial.println("[INFO] <- player pos and look X: " + String(mc.x)
                                        + " Y: " + String(mc.y)
                                        + " Z: " + String(mc.z)
                                        + " yaw: " + String(mc.yaw)
                                        + " pitch: " + String(mc.pitch)
                                        + " flags: " + String(flags)
                                        + " id: " + String(id));

                        mc.teleportConfirm(client, id);
                        break;
                    }
                    case 0x09:{
                        int ent_id = mc.readVarInt(client);
                        long pos = mc.readLong(client);
                        uint8_t stage = client.read();
                        Serial.println("[INFO] <- block destroy id: " + String(ent_id)
                                        + " pos: " + String(pos)
                                        + " stage: " + String(stage));
                        break;
                    }
                    case 0x21:{
                        long num = mc.readLong(client);
                        mc.keepAlive(client, num);
                        Serial.println("[INFO] <- received keepalive " + String(num));
                        break;
                    }
                    case 0x0F:{
                        String chat = mc.readString(client);
                        uint8_t pos = client.read();
                        Serial.println("[INFO] <- received message: " + chat + " sender type: " + String(pos));
                    }
                    default:{
                        for(int i=0; i < pack_length - mc.VarIntLength(id) - mc.VarIntLength(data_length); i++ ){
                            while (client.available() < 1);
                            client.read();
                        }
                        break;
                    }
                }
            } 
        } else {
            int id = mc.readVarInt(client);

            Serial.print("[INFO] <- Received packet length: " + String(pack_length) + " bytes packet id: 0x");
            Serial.println(id, HEX);

            switch (id){
                case 0x03:{
                    int tres = mc.readVarInt(client);
                    mc.compression_treshold = tres;
                    mc.compression_enabled = true;

                    Serial.println("[INFO] * Compression treshold set to " + String(tres) + " bytes");

                    break;
                }
                case 0x00:{
                    String str = mc.readString(client);
                    Serial.println("[INFO] <- text Received: " + str);
                    break;
                }
                default:{
                    for(int i=0; i<pack_length - 1; i++ ){
                        client.read();
                    }
                    break;
                }
            }
        }
    }
}