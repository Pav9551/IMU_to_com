#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#define PIN 0 // номер порта к которому подключен LED 0
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800); //first number change does distance between colors 
//Direccion I2C de la IMU
#define MPU 0x68
 
//Ratios de conversion
#define A_R 16384.0 // 32768/2
#define G_R 131.0 // 32768/250
 
//Conversion de radianes a grados 180/PI
#define RAD_A_DEG = 57.295779
 
//MPU-6050 da los valores en enteros de 16 bits
//Valores RAW
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
 
//Angulos
float Acc[2];
float Gy[3];
float Angle[3];

String valores;

long tiempo_prev;
float dt;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

const char* ssid = "****";
const char* password = "****";
IPAddress ip(192, 168, 99, 79);
IPAddress gateway(192, 168, 173, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];
#define MESSAGE_SIZE 1*sizeof(unsigned short) + \
            6*sizeof(signed long int) + \
            sizeof(unsigned short) + \
            4*sizeof(unsigned char) // CRC and header

#pragma pack(push, 1)//установили размер выравнивания в 1 байт
typedef struct _BINS_Message
{
    unsigned short header;
    signed long int Gx;
    signed long int Gy;
    signed long int Gz;
    signed long int Ax;
    signed long int Ay;
    signed long int Az;
    unsigned short ADD;
    unsigned char Counter;
    unsigned char St;
    unsigned char crc1;
    unsigned char crc2;

} BINS_Message;
#pragma pack(pop) //обратно установили размер выравнивания


union _BINS_union
    {
        
        unsigned char buf[32]{0xc0, 0xc0, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x89, 0xff, 0x02, 0x00, 0xfe, 0x06, 0x00, 0x00, 0x7e, 0x02, 0x00, 0x00, 0x45, 0x12, 0xbd, 0x60, 0x1d, 0xf1};
        BINS_Message state;
    } BINS_union;
static BINS_Message state;
_BINS_union state_union;
_BINS_union data;
unsigned char Count = 0;
uint32_t timer;
int calcrc(unsigned char *ptr, int count)
{
    int  crc;
    char i;
    crc = 0xffff;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}
void setup() {
  
  pixels.begin();
  pixels.show(); // Устанавливаем все светодиоды в состояние "Выключено"
  pixels.setPixelColor(0, pixels.Color(0,0,050));
  pixels.show(); 
  Wire.begin(4,5); // D2(GPIO4)=SDA / D1(GPIO5)=SCL
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //WIFI_AP_STA
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {
    delay(500);
  }
  if (i == 21) {
    Serial.print("Could not connect to"); Serial.println(ssid);
    while (1) {
      delay(500);
    }
  }
  //start UART and the server
  Serial.begin(115200);
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void loop() {
  
     //Leer los valores del Acelerometro de la IMU
   Wire.beginTransmission(MPU);
   Wire.write(0x3B); //Pedir el registro 0x3B - corresponde al AcX
   Wire.endTransmission(false);
   Wire.requestFrom(MPU,6,true);   //A partir del 0x3B, se piden 6 registros
   AcX=Wire.read()<<8|Wire.read(); //Cada valor ocupa 2 registros
   AcY=Wire.read()<<8|Wire.read();
   AcZ=Wire.read()<<8|Wire.read(); 
 
   //A partir de los valores del acelerometro, se calculan los angulos Y, X
   //respectivamente, con la formula de la tangente.
   Acc[1] = atan(-1*(AcX/A_R)/sqrt(pow((AcY/A_R),2) + pow((AcZ/A_R),2)))*RAD_TO_DEG;
   Acc[0] = atan((AcY/A_R)/sqrt(pow((AcX/A_R),2) + pow((AcZ/A_R),2)))*RAD_TO_DEG;
 
   //Leer los valores del Giroscopio
   Wire.beginTransmission(MPU);
   Wire.write(0x43);
   Wire.endTransmission(false);
   Wire.requestFrom(MPU,6,true);   //A partir del 0x43, se piden 6 registros
   GyX=Wire.read()<<8|Wire.read(); //Cada valor ocupa 2 registros
   GyY=Wire.read()<<8|Wire.read();
   GyZ=Wire.read()<<8|Wire.read();
 
   //Calculo del angulo del Giroscopio
   Gy[0] = GyX/G_R;
   Gy[1] = GyY/G_R;
   Gy[2] = GyZ/G_R;

   dt = (millis() - tiempo_prev) / 1000.0;
   tiempo_prev = millis();
   unsigned short dt_mks =(unsigned short)(micros() - timer); // Calculate delta time
   timer = micros();
   //Aplicar el Filtro Complementario
   Angle[0] = 0.98 *(Angle[0]+Gy[0]*dt) + 0.02*Acc[0];
   Angle[1] = 0.98 *(Angle[1]+Gy[1]*dt) + 0.02*Acc[1];

   //Integración respecto del tiempo paras calcular el YAW
   Angle[2] = Angle[2]+Gy[2]*dt;
   if (Angle[2] > 360) {Angle[2] = 0;}
   //Mostrar los valores por consola
   valores = "30," +String(Angle[0]) + "," + String(Angle[1])  + ",-30";//+ "," + String(Angle[2])
   valores = String(Angle[0]) + "," + String(Angle[1]);//+ "," + String(Angle[2])
   //serverClients[0].println(valores);
   
   //delay(10);
  unsigned char buf1[32]{0xc0, 0xc0,0x94, 0xef,0xff, 0xff,0x89, 0xf3,0xff, 0xff,0x57, 0x11,0x00, 0x00,0x0b,0xff, 0x02,0x00,0xfe, 0x06,0x00,0x00, 0x7e,0x02, 0x00, 0x00, 0x45,0x12, 0xbd,0x60, 0x1d,0xf1}; 
  memcpy(state_union.buf,buf1, sizeof(state_union.buf));
        state_union.state.Gx = GyX;
        state_union.state.Gy = GyY;
        state_union.state.Gz = GyZ; 
        state_union.state.Ax = AcX;
        state_union.state.Ay = AcY;
        state_union.state.Az = AcZ; 
        state_union.state.St = Count;
        state_union.state.Counter = Count;
        state_union.state.ADD= dt_mks;
        unsigned short crc = calcrc(state_union.buf+2, MESSAGE_SIZE  - sizeof(unsigned short) - 2*sizeof(unsigned char));
        unsigned char crc1 = (unsigned char) (crc & 0x00FF);
        unsigned char crc2 = (unsigned char) ((crc >> 8) & 0x00FF);
        state_union.state.crc1= crc1;
        state_union.state.crc2= crc2;
        Count = Count + 1;
        if (Count == 256) Count = 0; 
              
  serverClients[0].write(state_union.buf,sizeof(state_union.buf));
  delay(1);
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        Serial.print("New client: "); Serial.print(i);
        pixels.setPixelColor(0, pixels.Color(50,00,0));//r_g
        if (serverClients[i]) {         
          serverClients[i].stop();
        }
        serverClients[i] = server.available();
        Serial.print("New client: "); Serial.print(i);
        pixels.setPixelColor(0, pixels.Color(0,50,0));//r_g
        
        pixels.show(); 
        break;
      }
    }
    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      WiFiClient serverClient = server.available();
      serverClient.stop();
      Serial.println("Connection rejected ");
    }
  }
  //check clients for data
 /* for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if (serverClients[i].available()) {
        //get data from the telnet client and push it to the UART
        while (serverClients[i].available()) {
          Serial.write(serverClients[i].read());
        }
      }
    }
  }*/
  //check UART for data
    
  /*if (Serial.available()) {
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      
      if (serverClients[i] && serverClients[i].connected()) {
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
    
  }*/
}
