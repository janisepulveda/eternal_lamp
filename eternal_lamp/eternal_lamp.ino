#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include "NewPing.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ESP8266))   // Using a soft serial port
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/rx =/10, /tx =/11);
#define FPSerial softSerial
#else
#define FPSerial Serial1
#endif


#define TRIGGER_PIN 8
#define ECHO_PIN 9
#define MAX_DISTANCE 60
#define RELAY_PIN 4

NewPing sonar (TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
DFRobotDFPlayerMini myDFPlayer;

int audioActual = 0;
const int totalAudios = 5;
bool audioIniciado = false;

// State constants
enum State {
  STANDBY,
  RELAY_ON,
  PLAY_SONG,
  RELAY_OFF,
  WAIT,
};
char *estadosTxt[] = {
  "STANDBY",
  "RELAY_ON",
  "PLAY_SONG",
  "RELAY_OFF",
  "WAIT"
};
State estadoActual = RELAY_ON;
unsigned long stateStartTime;
const unsigned long relayDuration = 2000;  // 2 seconds
const unsigned long songDuration = 10000;  // 5 seconds
const unsigned long relayOffDuration = 2000;  // 2 seconds
const unsigned long waitDuration = 1000;  // 10 seconds

void relay(bool estadoLuz) {
  // esta funcion activa y desactiva la conexion del relay para la luz
  // escribir en el pin del relay el valor que queremos (en SIGNAL) dependiendo del booleano de entrada
  if (estadoLuz){
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
}

bool usuarioDetectado() {
    int distancia = sonar.ping_cm();
    return (distancia >= 20 && distancia <= 100);
    //return true;
}

void configurarMP3() {
  #if (defined ESP32)
    FPSerial.begin(9600, SERIAL_8N1, /rx =/D3, /tx =/D2);
  #else
    FPSerial.begin(9600);
  #endif

    Serial.println();
    Serial.println(F("DFRobot DFPlayer Mini Demo"));
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true)) {  //Use serial to communicate with mp3.
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1.Please recheck the connection!"));
      Serial.println(F("2.Please insert the SD card!"));
      /*while (true) {
        delay(0);  // Code to compatible with ESP8266 watch dog.
      }*/
      
    }
    Serial.println(F("DFPlayer Mini online."));

    myDFPlayer.volume(30);  //Set volume value. From 0 to 30
    //myDFPlayer.play(1);     //Play the first mp3
}

void reproducirMP3(){
  //myDFPlayer.play(3);
  myDFPlayer.play(random(1,6)); // entre 1 y 9
  audioIniciado = true;
}

void setup() {
  Serial.begin(115200);
  configurarMP3();

  Serial.println("Sistema en standby");
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
  Serial.print("Estado: "); Serial.println(estadosTxt[estadoActual]);
  switch (estadoActual) {
    case STANDBY:
      if (usuarioDetectado()) {
        Serial.println("Hay usuario");
        Serial.println("Cambiar a RELAY_ON");
        Serial.println("Se encendera la luz por 2 segundos");
        estadoActual = RELAY_ON;
        stateStartTime = millis();
        audioActual = (audioActual % totalAudios)+1;
      } else {
        relay(false);
      }
      break;

    case RELAY_ON:
      //if (!usuarioDetectado()) { estadoActual = STANDBY; }
      relay(true);
      if (millis() - stateStartTime >= relayDuration) {
        Serial.println("Luz estuvo encendida por 2 segundos.");
        Serial.println("Cambiar a PLAY_SONG");
        estadoActual = PLAY_SONG;
        stateStartTime = millis();
      }
      break;

    case PLAY_SONG:
      //if (!usuarioDetectado()) { estadoActual = STANDBY; }
      if (!audioIniciado) {
       reproducirMP3();
      }
            
      if (millis() - stateStartTime >= songDuration) {
        Serial.println("Audio finalizado, vamos a apagar la luz en  RELAY_OFF");
        estadoActual = RELAY_OFF;
        stateStartTime = millis();
        audioIniciado = false;
      }
      break;

    case RELAY_OFF:
      //if (!usuarioDetectado()) { estadoActual = STANDBY; }
      relay(false);

      if (millis() - stateStartTime >= relayOffDuration) {
        Serial.println("Luz apagada después de 2seg, vamos a WAIT");
      
        estadoActual = WAIT;
        stateStartTime = millis();
      }
      break;

    case WAIT:
      //if (!usuarioDetectado()) { estadoActual = STANDBY; }

      if (millis() - stateStartTime >= waitDuration) {
        Serial.println("Espera final OK, volver a STANDBY");
        estadoActual = STANDBY;
      }
      break;
  }

  delay(10);
}