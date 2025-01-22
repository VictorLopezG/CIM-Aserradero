#include "EmonLib.h"
#include "FS.h"
#include "SPI.h"
#include "SD.h"
#include <RTClib.h>

// Crear una instancia EnergyMonitor
EnergyMonitor energyMonitor;
RTC_DS1307 rtc;
//EnergyMonitor energyMonitor1;
// Voltaje de nuestra red eléctrica
float voltajeRed = 116.0;
const int CS = 5;          // Chip Select de la SD
int errorCount = 0;        // Contador de errores
const int maxErrors = 20;  // Número máximo de errores permitidos antes de reiniciar
double maximo = 0.0;
int a, b;
bool corte=false;
unsigned long timer = 0;
bool led = true;
unsigned long inicio,fin;

void writeFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void RTC_init() {
  rtc.begin();
  Serial.println(rtc.isrunning() ? "RTC no funciona" : "RTC funcionando");
  if (rtc.!isrunning()) {
    Serial.println("Ajustando el reloj interno...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    a = 1;
  } else {
    b = 1;
  }
  Serial.flush();
}

void setup() {
  Serial.begin(115200);
  pinMode(14, OUTPUT);

  if (!SD.begin(CS)) {
    Serial.println("Error al inicializar la SD");
    //while (1)  ;
  }
  Serial.println("SD inicializada correctamente");

  RTC_init();

  if (!SD.exists("/corr.txt")) {
    writeFile(SD, "/corr.txt", "Fecha Hora Corriente(A)");
  }
  energyMonitor.current(2, 11.11);
}

void loop() {
  double Irms = energyMonitor.calcIrms(1484);

  if (millis() - timer >= 1000) {
    if (led == true) {
      digitalWrite(14, HIGH);
    } else {
      digitalWrite(14, LOW);
    }
    led = !led;
    timer = millis();
  }

  DateTime now = rtc.now();
  String fecha = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

  double corriente1 = Irms*1.7;
  if(corriente1>0.2)corriente1 -=0.2;
  String corr = String(corriente1);
  String Datos = fecha + " " + corr;
  Datos+='\n';
  Serial.print(Datos);
  
  //guardar datos
  appendFile(SD, "/corr.txt", Datos.c_str());
}
