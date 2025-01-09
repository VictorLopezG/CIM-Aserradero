#include "Adafruit_VL53L0X.h"
#include <Wire.h>
#include <SD.h>
#include "RTClib.h"

int mill;
unsigned long t_offset = 0;
unsigned long time_out = 0;
// Configuración del sensor y SD
#define VL53_ADDRESS 0x29
#define SD_CS_PIN 5  // Pin Chip Select para la SD

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
RTC_DS1307 rtc;

VL53L0X_RangingMeasurementData_t measure;

int distanceThreshold = 200;         // Umbral en mm para detectar un orificio (mm)
unsigned long timeThreshold = 1800;  // Tiempo antes del timeout (ms)
volatile int pulseCount = 0;         // Contador de orificios detectados
unsigned long lastEventTime = 0;     // Tiempo del último evento (time)
unsigned long currentTime = 0;       // Tiempo actual
bool holeDetected = false;           // Estado del orificio detectado
bool led = true;

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    } else {
      Serial.println("Write failed");
    }
  file.close();
}

// Inicialización del sensor VL53L0X
void initLaser() {
  if (!lox.begin(VL53_ADDRESS)) {
    Serial.println("No se pudo iniciar el sensor VL53L0X");
    while (1);
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
void saveToSD(int pulse, bool flag = false) {
  File file = SD.open("/velocidad.txt", FILE_APPEND);
  String milli;
  if(flag){
    milli = "Stopped";
  }else{
    milli = String(float(millis()-mill));
  }
  if (file) {
    DateTime now = rtc.now();
    String time = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " " + milli;
    file.println(time);
    file.close();
    Serial.println(time);
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
    writeFile(SD, "/velocidad.txt", "Date Time Milliseconds\n");
  }
}

unsigned long timer;

void loop() {
  // Parpadeo LED
  if (millis() - timer > 1000) {
    if (led == true) {
      digitalWrite(14, HIGH);
    } else {
      digitalWrite(14, LOW);
    }
    led = !led;
    timer = millis();
  }

  // Si no ha pasado el timeout
  currentTime = millis();
  if (time_out < timeThreshold){
    lox.rangingTest(&measure, false);  // Realiza la medición

    // Detecta un orificio
    if (measure.RangeMilliMeter < distanceThreshold && !holeDetected) {
      holeDetected = true;  // Se detecta un orificio
      pulseCount++;
      // Calcula el tiempo transcurrido desde el último evento
      if (lastEventTime > 0) { // Si no acaba de iniciar el programa
        saveToSD(pulseCount);  // Guarda los pulsos y la velocidad en la tarjeta SD
        Serial.print("Pulsos: ");
        Serial.println(pulseCount);
        time_out = 0;
        t_offset = currentTime;
      }
      lastEventTime = currentTime;  // Actualiza el tiempo del último evento
    }
    // Detecta cuando ya no hay orificio
    if (measure.RangeMilliMeter >= distanceThreshold) {
      holeDetected = false;
    }
  // Pasó la ventana de tiempo sin detectar pulsos
  }else{
    if (lastEventTime > 0) { // Si no acaba de iniciar el programa
      saveToSD(pulseCount, true);  // Guarda los pulsos y la velocidad en la tarjeta SD
      Serial.print("Pulsos: ");
      Serial.println(pulseCount);
      time_out = 0;
      t_offset = currentTime;
    }
  }

  time_out = currentTime - t_offset;
}
