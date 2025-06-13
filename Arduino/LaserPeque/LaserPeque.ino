// Modulo Laser pequeño
#include "Adafruit_VL53L0X.h"
#include <RTClib.h>
#include <Wire.h>
#include <math.h>
#include "RTClib.h"
#include <Adafruit_MLX90614.h>
#include <Adafruit_ADS1X15.h>
#include <SoftwareSerial.h>
#include <CRC16.h>
#include <SD.h>
//puertos para el laser grande
#define TX_laser 17
#define RX_laser 16
//sensor de temperatura
#define MLX_ADDRESS 0x5A
//laser
#define VL53_ADDRESS 0x29
//modulo ads
#define ADS1_ADDRESS 0x48
//reloj
RTC_DS1307 rtc;
SoftwareSerial mySerial(TX_laser, RX_laser);
//comprobacion de datos
CRC16 crc(CRC16_MODBUS_POLYNOME,
          CRC16_MODBUS_INITIAL,
          CRC16_MODBUS_XOR_OUT,
          CRC16_MODBUS_REV_IN,
          CRC16_MODBUS_REV_OUT);

double Temp;
VL53L0X_RangingMeasurementData_t measure1;
int a = 0;
int b = 0;
bool led = true, sd_check = false;

Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
//inicialización de la biblioteca modulo ads y sensor temp
Adafruit_ADS1115 ads;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
//comandos para comunicacion con laser grande
uint8_t byte6_1, byte5_1, byte6_2, byte5_2, byte5_1_hex, byte5_2_hex, byte6_1_hex, byte6_2_hex;

uint8_t commandContinue[8] = { 0x01, 0x03, 0x00, 0x01, 0x00, 0x02, 0x95, 0xCB };
uint8_t commandUnique[8] = { 0x01, 0x03, 0x00, 0x0F, 0x00, 0x02, 0xF4, 0x08 };
uint8_t marche[11] = { 0x01, 0x10, 0x00, 0x03, 0x00, 0x01, 0x02, 0x00, 0x01, 0x67, 0xA3 };
uint8_t data[9];
uint32_t distance;
//calculo de la distancia
uint32_t calcul_distance(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  distance = (d * 1 + c * 16 + b * 16 * 16 + a * 16 * 16 * 16);  //converts the 4 hex digits into decimal
  return distance;
}
//iniciar reloj
void RTC_init() {
  rtc.begin();
  Serial.println(!rtc.isrunning() ? "RTC no funciona" : "RTC funcionando");
  if (!rtc.isrunning()) {
    Serial.println("Ajustando el reloj interno...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    a = 1;
  } else {
    b = 1;
  }
  Serial.flush();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if(file){
    Serial.println("Archivo no se pudo abrir");
    sd_check = false;
  } else {
    Serial.println("Archivo abierto");
    sd_check = true;
  }
  Serial.println(file.print(message) ? "Archivo escrito" : "No se ha podido escribir");
  file.close();
}
void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  Serial.print(!file ? "Archivo no se pudo abrir\n":"");
  Serial.print(!file.print(message) ? "No se ha podido añadir\n":"");
  file.close();
}
//iniciar SD
void SD_init() {
  if(!SD.begin()){
    Serial.println("No se puede encontrar la SD");
    sd_check = false;
  } else {
    Serial.println("Se encontro la SD");
    sd_check = true;
  }
  File file = SD.open("/Datos.txt");
  Serial.println(!file ? "El archivo no existe, se creara uno" : "El archivo ya existe");
  if (!file) {
    writeFile(SD, "/Datos.txt", "-WPMS5003\r\n");
  } else {
    appendFile(SD, "/Datos.txt", "-APMS5003\r\n");
  }
  file.close();
}

String getDate() {
  DateTime now = rtc.now();
  String dateTime = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  // Serial.println(dateTime);
  return dateTime;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  Serial.print("start");
  mySerial.write(marche, 11);
  Wire.begin();
  lox1.begin(VL53_ADDRESS);
  mlx.begin(MLX_ADDRESS);
  ads.begin(ADS1_ADDRESS);
  pinMode(14, OUTPUT);

  RTC_init();

  if (!SD.begin(5)) {
    Serial.println("Initialization SD failed!");
    sd_check = false;
    return;
  } else {
    sd_check = true;
  }
  if (!SD.exists("/Datos.txt")) {
    File myFile = SD.open("/Datos.txt", FILE_WRITE);
    myFile.println("Fecha Hora Temperatura Distancia Laser_pequeño");
    myFile.close();
  }
}

unsigned long timer = 0;

void loop() {

  if (millis() - timer > 1000) {
    mySerial.write(commandUnique, 8);
    // delay(50);
    if (mySerial.available() > 0) {
      mySerial.readBytes(data, 9);
      mySerial.overflow();
    }
    Temp = mlx.readObjectTempC();  //Temperatura obtenida del sensor de temperatura
    //Serial.print("Temperatura Objeto: ");
    // Serial.println(Temp);
    lox1.rangingTest(&measure1);

    for (int i = 0; i < sizeof(data); i++) {
      crc.add(data[i]);
    }
    uint8_t CRC = crc.calc();
    //Serial.println(CRC, HEX);

    // delay(50);
    byte5_1 = data[5] / 16;
    byte5_2 = data[5] - byte5_1 * 16;
    byte6_1 = data[6] / 16;
    byte6_2 = data[6] - byte6_1 * 16;
    distance = calcul_distance(byte5_1, byte5_2, byte6_1, byte6_2);

    crc.restart();
    timer = millis();
    String fecha = getDate();
    
    int contador;
    //visualizar y guardar los datos
    if (distance != 0 && !isnan(Temp) && distance < 20000 && CRC == 0) {
      Serial.println("------- Datos del Sensor -------");
      Serial.print("Fecha: ");
      Serial.println(fecha);  // Asegúrate de que 'fecha' esté definida y tenga un formato válido
      Serial.print("Temperatura: ");
      Serial.print(Temp);
      Serial.println(" ºC");
      Serial.print("Distancia: ");
      Serial.print(distance);
      Serial.println(" mm");
      Serial.print("Medidas Laser Pequeño: ");
      Serial.print(measure1.RangeMilliMeter);
      Serial.println(" mm");
      
      String datosSD = fecha + " " + String(Temp) + " " + String(distance) + " " + String(measure1.RangeMilliMeter) + "\n";
      appendFile(SD, "/Datos.txt", datosSD.c_str());

      // Parpadeo del led
      if (led == true) {
        digitalWrite(14, HIGH);
      } else {
        if(sd_check){ digitalWrite(14, LOW);}
      }
      led = !led;

    } else {
      Serial.println("Datos se estan recibiendo mal");
      Serial.println("Distancia= " + String(distance) + " Temperatura = " + String(Temp) + " CRC =" + String(CRC));
      contador++;
      if (contador >= 35) {
        ESP.restart();
      }
      // Apagar led
      digitalWrite(14, LOW);
    }
  }
}
