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

void setup() {
  Serial.begin(9600);
  //mlx.begin();

  pinMode(14, OUTPUT);

  if (!SD.begin(CS)) {
    Serial.println("Error al inicializar la SD");
    while (1)
      ;
  }
  Serial.println("SD inicializada correctamente");

  RTC_init();

  if (!SD.exists("/corr.txt")) {
    writeFile(SD, "/corr.txt", "Fecha Corriente1 Corriente2");
  }
  energyMonitor.current(2, 11.11);
  //energyMonitor3.current(1, 111.1);
  //energyMonitor1.current(1, 3.03);
}

unsigned long timer = 0;
bool led = true;

void loop() {
  double Irms = energyMonitor.calcIrms(1484);
  //    double Irms2 = energyMonitor2.calcIrms(1484);
  //double Irms3 = energyMonitor3.calcIrms(1484);

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

  // Verificar si el archivo existe
  double corriente1 = Irms*1.7 -0.2;
  //double corriente2 = Irms2;
  //double corriente3 = Irms3 * 0.58;*/
  String corr = String(corriente1);
  //String corr2=String(corriente2);
  corr.replace('.', ',');
  //corr2.replace('.',',');
  String Datos = fecha + " " + corr + '\n';  // + " " + String(corriente2) + " " + String(corriente3);
  Serial.print(Datos);
  //Serial.print(analogRead(2));

  // Abrir archivo para agregar datos
  if (!fecha) {
    Serial.println("Fecha no obtenida");
  } else {
    appendFile(SD, "/corr.txt", Datos.c_str());
  }

  /*/ Reiniciar si hay demasiados errores
    if (errorCount >= maxErrors) {
        Serial.println("Demasiados errores. Reiniciando...");
        asm volatile("jmp 0");
    }

    // delay(1000); // Esperar 1 segundo antes de repetir*/
}
