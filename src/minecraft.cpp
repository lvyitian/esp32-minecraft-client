#include "minecraft.h"
#include <Arduino.h>

// public //

minecraft::minecraft(String _username, String _url, uint16_t _port){
    username = _username;
    server_url = _url;
    server_port = _port;
}

void minecraft::teleportConfirm(Stream& S, int id){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(S, 2 + VarIntLength(id));
        writeVarInt(S, 0); // empty data length
        writeVarInt(S, 0x00);
    } else {
        writeVarInt(S, 1 + VarIntLength(id));
        writeVarInt(S, 0x00);
    }
    writeVarInt(S, id);
    writing = false;
}

void minecraft::setRotation(Stream& S, float yaw, float pitch, bool ground){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(S, 11);
        writeVarInt(S, 0); // empty data length
        writeVarInt(S, 0x13);
    } else {
        writeVarInt(S, 10);
        writeVarInt(S, 0x13);
    }
    writeFloat(S, yaw);
    writeFloat(S, pitch);
    S.write(ground?1:0);
    writing = false;
}

void minecraft::keepAlive(Stream& S, uint64_t id){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(S, 10);
        writeVarInt(S, 0); // empty data length
        writeVarInt(S, 0x0F);
    } else {
        writeVarInt(S, 9);
        writeVarInt(S, 0x0F);
    }
    writeLong(S, id);
    writing = false;
}

void minecraft::request(Stream& S){
    writeVarInt(S, 1);
    writeVarInt(S, 0);
}

void minecraft::ping(Stream& S, uint64_t num){
    if(compression_enabled){
        writeVarInt(S, 10);  //packet lenght
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 1);  //packet id
    } else {
        writeVarInt(S, 9);  //packet lenght
        writeVarInt(S, 1);  //packet id
    }
    writeLong(S, num);
}

void minecraft::loginStart(Stream& S){
    if(compression_enabled){
        writeVarInt(S, 3 + username.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 0);
    } else {
        writeVarInt(S, 2 + username.length());
        writeVarInt(S, 0); 
    }
    writeString(S, username);
}

void minecraft::writeChat(Stream& S, String text){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(S, 3 + text.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 3);
    } else {
        writeVarInt(S, 2 + text.length());
        writeVarInt(S, 3); 
    }
    writeString(S, text);
    writing = false;
}

void minecraft::handShake(Stream& S, uint8_t state){
    if(compression_enabled){
        writeVarInt(S, 8 + server_url.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 0);
    } else {
        writeVarInt(S, 7 + server_url.length());
        writeVarInt(S, 0);
    }
    writeVarInt(S, 578);
    writeString(S, server_url);
    writeUnsignedShort(S, server_port);
    writeVarInt(S, state);
}

void minecraft::clientStatus(Stream& S, uint8_t state){
    if(compression_enabled){
        writeVarInt(S, 3);
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 4);
    } else {
        writeVarInt(S, 2);
        writeVarInt(S, 4);
    }
    writeVarInt(S, 0);
}

float minecraft::readFloat(Stream& S){
    char b[4] = {};
    while(S.available() < 4);
    for(int i=3; i>=0; i--){
        b[i] = S.read();
        Serial.print(" ");
        Serial.print(b[i], HEX);
    }
    Serial.print("  ");
    float f = 0;
    memcpy(&f, b, sizeof(float));
    Serial.println(f);
    return f;
}

double minecraft::readDouble(Stream& S){
    char b[8] = {};
    while(S.available() < 8);
    for(int i=7; i>=0; i--){
        b[i] = S.read();
        Serial.print(" ");
        Serial.print(b[i], HEX);
    }
    Serial.print("  ");
    double d = 0;
    memcpy(&d, b, sizeof(double));
    Serial.println(d);
    return d;
}

long minecraft::readLong(Stream& S){
    char b[8] = {};
    while(S.available() < 8);
    for(int i=0; i<8; i++){
        b[i] = S.read();
    }
    long l = ((long) b[0] << 56)
       | ((long) b[1] & 0xff) << 48
       | ((long) b[2] & 0xff) << 40
       | ((long) b[3] & 0xff) << 32
       | ((long) b[4] & 0xff) << 24
       | ((long) b[5] & 0xff) << 16
       | ((long) b[6] & 0xff) << 8
       | ((long) b[7] & 0xff);
    return l;
}

String minecraft::readString(Stream& S){
    int length = readVarInt(S);
    Serial.println("[INFO] <- text length to read: " + String(length) + " bytes");
    String result;
    for(int i=0; i<length; i++){
        while (S.available() < 1);
        result.concat((char)S.read());
    }
    return result;
}

int minecraft::readVarInt(Stream& S) {
    int numRead = 0;
    int result = 0;
    byte read;
    do {
        while (S.available() < 1);
        read = S.read();
        int value = (read & 0b01111111);
        result |= (value << (7 * numRead));
        numRead++;
        if (numRead > 5) {
            Serial.println("[ERROR] VarInt too big");
        }
    } while ((read & 0b10000000) != 0);
    return result;
}

// private //
int minecraft::VarIntLength(int val) {
    if(val == 0){
        return 1;
    }
    return (int)floor(log(val) / log(128)) + 1;
}

void minecraft::writeDouble(Stream& S, double value){
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=7; i>=0; i--){
        S.write(p[i]);
    }
}

void minecraft::writeFloat(Stream& S, float value) {
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=3; i>=0; i--){
        S.write(p[i]);
    }
}

void minecraft::writeVarInt(Stream& S, int16_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S.write(temp);
    } while (value != 0);
}

void minecraft::writeVarLong(Stream& S, int64_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S.write(temp);
    } while (value != 0);
}

void minecraft::writeString(Stream& S, String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(S, length);
    for(int i=0; i<length; i++){
        S.write(buf[i]);
    }
}

void minecraft::writeLong(Stream& S, uint64_t num){
    for(int i=7; i>=0; i--){
        S.write((byte)((num >> (i*8)) & 0xff));
    }
}

void minecraft::writeUnsignedShort(Stream& S, uint16_t num){
    S.write((byte)((num >> 8) & 0xff));
    S.write((byte)(num & 0xff));
}

int lsr(int x, int n){
  return (int)((unsigned int)x >> n);
}