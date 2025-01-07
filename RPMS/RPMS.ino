#include "Adafruit_VL53L0X.h"
#include <Wire.h>
#include <SD.h>
#include "RTClib.h"

int mill;
// Configuración del sensor y SD
#define VL53_ADDRESS 0x29
#define SD_CS_PIN 5  // Pin Chip Select para la SD

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
RTC_DS1307 rtc;

VL53L0X_RangingMeasurementData_t measure;

int threshold = 200;              // Umbral en mm para detectar un orificio
volatile int pulseCount = 0;      // Contador de orificios detectados
unsigned long lastEventTime = 0;  // Tiempo del último evento
float rpm = 0;                    // Velocidad calculada en RPM
bool holeDetected = false;        // Estado del orificio detectado
bool led = true;

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

// Inicialización del sensor VL53L0X
void initLaser() {
  if (!lox.begin(VL53_ADDRESS)) {
    Serial.println("No se pudo iniciar el sensor VL53L0X");
    while (1)
      ;
  }
  Serial.println("Sensor VL53L0X iniciado.");
}

// Inicialización del RTC
void initRTC() {
  if (!rtc.begin()) {
    Serial.println("No se pudo iniciar el RTC.");
    while (1)
      ;
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC detenido, ajustando la hora...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Configura la hora actual
  }
  Serial.println("RTC iniciado.");
   int seg = rtc.now().second();
  while (seg == rtc.now().second()) {
    delay(1);
  }
  mill = millis();
}

// Inicialización de la tarjeta SD
void initSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error al inicializar la tarjeta SD.");
  }
  Serial.println("Tarjeta SD inicializada.");
}

// Guardar datos en la SD
void saveToSD(int pulse) {
  File file = SD.open("/velocidad.txt", FILE_APPEND);
  String milli=String(float((millis()-mill))/1000);
  milli=milli.substring(milli.indexOf('.')+1);
  if (file) {
    DateTime now = rtc.now();
    file.print(String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " "+milli);
    file.println(pulse);
    file.close();
    Serial.println("Datos guardados en SD.");
  } else {
    Serial.println("No se pudo abrir el archivo.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(14, OUTPUT);

  initLaser();  // Inicia el sensor VL53L0X
  initRTC();    // Inicia el RTC
  initSD();     // Inicia la tarjeta SD

  if (!SD.exists("/velocidad.txt")) {
    writeFile(SD, "/velocidad.txt", "Date Time Pulse\n");
  }
}

unsigned long timer;

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

  lox.rangingTest(&measure, false);  // Realiza la medición

  // Detecta un orificio
  if (measure.RangeMilliMeter < threshold && !holeDetected) {
    holeDetected = true;  // Se detecta un orificio
    pulseCount++;

    // Calcula el tiempo transcurrido desde el último evento
    unsigned long currentTime = millis();
    if (lastEventTime > 0) {
      /*unsigned long timeDiff = currentTime - lastEventTime; // Diferencia en ms
      float periodInSeconds = timeDiff / 1000.0;
      rpm = (1.0 / periodInSeconds) * 60.0 / 6.0; // Dividir entre 6 orificios*/
      saveToSD(pulseCount);  // Guarda los pulsos y la velocidad en la tarjeta SD
      Serial.print("Pulsos: ");
      Serial.println(pulseCount);
    }
    lastEventTime = currentTime;  // Actualiza el tiempo del último evento
  }

  // Detecta cuando ya no hay orificio
  if (measure.RangeMilliMeter >= threshold) {
    holeDetected = false;
  }
}
