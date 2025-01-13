// Modulo Laser reparado
#include <SoftwareSerial.h>
#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

RTC_DS1307 rtc;
DateTime lastValidDate;
unsigned long timer = millis();
bool reg = false, led = true, madera;

SoftwareSerial mySerial(16, 17);  //Define software serial, 3 is TX, 2 is RX
char buff[4] = { 0x80, 0x06, 0x03, 0x77 };
unsigned char data[11] = { 0 };
float dis = 0;
float ldis = 0;
float temp = 0;

void WriteFile(const char* path, const char* message) {
  File myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  } else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}
//Agregar datos a un archivo de la SD
void appendFile(const char* path, String message) {
  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    //Serial.println("Failed to open file for appending");
    return;
  }
  if (!file.print(message)) {
    //Serial.println("Append failed");
  }
  file.close();
}

void regMem() {
  DateTime now = rtc.now();
  String time = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  Serial.print(time + " ");
  appendFile("/registro_laser.txt", fyh + " " + String(temp) + " " + String(dis) + " " + String(madera) + "\n");
}

void distancia(void* pv) {
  while (1) {
    reg = false;
    //Serial.println("esperando datos");
    while (!mySerial.available()) {
    }  //Determine whether there is data to read on the serial
    delay(50);
    for (int i = 0; i < 11; i++) {
      data[i] = mySerial.read();
    }
    unsigned char Check = 0;
    for (int i = 0; i < 10; i++) {
      Check = Check + data[i];
    }
    Check = ~Check + 1;
    if (data[10] == Check) {
      if (data[3] == 'E' && data[4] == 'R' && data[5] == 'R') {
        Serial.println("Out of range");
        dis = -1;
      } else {
        float distance = (data[3] - 0x30) * 100 + (data[4] - 0x30) * 10 + (data[5] - 0x30) * 1 + (data[7] - 0x30) * 0.1 + (data[8] - 0x30) * 0.01 + (data[9] - 0x30) * 0.001;
        dis = distance * 1000;
        //Serial.println("hola");
        reg = true;
      }
    } else {
      Serial.println("Invalid Data!");
    }
    delay(20);
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  mlx.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //-------------- TODO:
  } else {
    lastValidDate = rtc.now();
  }
  //inicializacion de SD
  Serial.println("Initializing SD card...");
  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    //return;
  } else {
    Serial.println("initialization done.");
  }
  if (!SD.exists("/registro_laser.txt")) {
    Serial.println("creando registro");
    WriteFile("/registro_laser.txt", "Fecha Hora Temperatura(ºC) Distancia(mm) Madera\n");
  }
  pinMode(14, OUTPUT);
  pinMode(25, INPUT);
  mySerial.print(buff);
  xTaskCreatePinnedToCore(
    distancia, /* Function to implement the task */
    "Task1",   /* Name of the task */
    10000,     /* Stack size in words */
    NULL,      /* Task input parameter */
    0,         /* Priority of the task */
    NULL,      /* Task handle. */
    0);
}

void loop() {

  if (millis() - timer > 1000) {
    if (led == true) {
      digitalWrite(14, HIGH);
    } else {
      digitalWrite(14, LOW);
    }
    led = !led;
    timer = millis();
  }
  madera = !digitalRead(25);
  temp = mlx.readObjectTempC();
  if (reg) {
    regMem();
    Serial.print(dis);
    Serial.print("mm\tObjeto =");
    Serial.print(temp);
    Serial.print("ºC ");
    Serial.print("Madera: ");
    Serial.println(madera);
  }
}
