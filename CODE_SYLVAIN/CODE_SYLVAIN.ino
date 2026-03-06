#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <ArduinoMDNS.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>   // Bibliothèque pour contrôler des bandes LED NeoPixel
#include <SharpIR.h>
#include <PubSubClient.h>

 
#define IRPin 5
#define model 430
 
int distance_cm;
 
/* Model :
  GP2Y0A02YK0F --> 20150
  GP2Y0A21YK0F --> 1080
  GP2Y0A710K0F --> 100500
  GP2YA41SK0F --> 430
*/
 
SharpIR mySensor = SharpIR(IRPin, model);
 


WiFiClient espClient;
PubSubClient client(espClient);
 
// Wi-Fi credentials
const char* ssid = "le routeur";
const char* password = "testtest";
const char* mqtt_server = "192.168.1.144";  // IP de ton PC ou broker Mosquitto
 
// WebServer to serve the web page
WebServer server(80);
 
// WebSocket server for real-time control
WebSocketsServer webSocket = WebSocketsServer(81);
 
// Motor control pins
#define GP8 9
#define GP9 8
#define GP10 10
#define GP11 11
#define M1_Backward 8
#define M1_Forward 9
#define M2_Forward 10
#define M2_Backward 11
 
#define LED_PIN   45
#define LED_PIN2  2
#define LED_COUNT 4
#define BRIGHTNESS 100
 
// Configuration des LED NeoPixel
#define RGB_IN  15              // Broche d'entrée pour les LED
#define Nbr_Pixels  4           // Nombre de pixels LED
#define Photores  28            // Broche pour le capteur de luminosité

// === Pins des encodeurs ===
const int pinA_L = 16;  // GP16
const int pinB_L = 17;  // GP17
const int pinA_R = 19;  // GP19
const int pinB_R = 20;  // GP20
 
volatile long pos_left = 0;
volatile long pos_right = 0;
volatile int lastA_L = 0;
volatile int lastA_R = 0;
 
unsigned long lastPublish = 0;
 
 
Adafruit_NeoPixel strip(Nbr_Pixels, RGB_IN, NEO_GRB + NEO_KHZ800); // Création d'un objet pour contrôler les LED NeoPixel
Adafruit_NeoPixel strip1(Nbr_Pixels, LED_PIN2, NEO_GRB + NEO_KHZ800); // Création d'un objet pour contrôler les LED NeoPixel
 
// Global variables
int speed_Backward = 0;
int speed_Forward = 0;
bool accelerating = false;
bool decelerating = false;
bool braking = false;
bool brakeActive = false;
bool modeTaxi = false;
bool taxiActif = false;
 
String current_light_cmd = "stop";
unsigned long previousBlinkTime = 0;
bool ledState = false;
 
unsigned long lastBlinkTime = 0;
bool blinkState = false;
 
bool clignotantGaucheActif = false;
bool clignotantDroiteActif = false;
const unsigned long blinkDelay = 300;
 
bool reculerOn = false;
unsigned long lastReculerBlink = 0;
bool reculerState = false;
 
unsigned long tempsRejoindreAutoroute = 0;
unsigned long tempsVersBureau = 0;
 
 
 
 
 
// États des différentes lumières
static int etat_lumiere_avant = 0; // Phares avant
static int etat_Clg_Gauche = 0;    // Clignotant gauche
static int etat_Clg_Droite = 0;    // Clignotant droit
static int etat_recul = 0;         // Lumières de recul
static int etat_frein = 0;         // Feux de freinage
 
unsigned long dernierTemps = 0;
int Etape = 1;
int departRangee, departBureau, destRangee, destBureau;
int tempsAttente = 0;
int ecartRangee;
String previousCommand = "";
int joystickX = 0;
int joystickY = 0;
int speed = 0;
int direction = 0;
 
 
// Variables pour le mode Taxi
int rangeeDepart = 0;
int bureauDepart = 0;
int rangeeArrivee = 0;
int bureauArrivee = 0;
 
 
int etapeTaxi = 0;
 
 
 
// Function prototypes
void neutre();
void avancer();
void reculer();
void tourner_gauche_avancer();
void tourner_droite_avancer();
void brake();
void Systeme_Clavier(String command);
void Systeme_automatique(String command);
void motorTest();
void setupWiFi();
void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
 
 
void brake() {
  if (braking) {
    neutre();
    Serial.println("Frein d'urgence!");
    braking = false;
    Etape = 0;
  }
}
 
void vitesse() {
  speed_Backward = 0;
  speed_Forward = 0;
}
 
void neutre() {
  digitalWrite(M1_Forward, LOW);
  digitalWrite(M1_Backward, LOW);
  digitalWrite(M2_Forward, LOW);
  digitalWrite(M2_Backward, LOW);
}
 
void avancer() {
  analogWrite(M1_Forward, speed_Forward);
  digitalWrite(M1_Backward, LOW);
  analogWrite(M2_Forward, speed_Forward);
  digitalWrite(M2_Backward, LOW);
}
 
void reculer() {
  analogWrite(M1_Forward, LOW);
  analogWrite(M1_Backward, speed_Forward);
  analogWrite(M2_Forward, LOW);
  analogWrite(M2_Backward, speed_Forward);
}
 
void tourner_gauche_avancer() {
  analogWrite(M1_Forward, speed_Forward);
  digitalWrite(M1_Backward, LOW);
  digitalWrite(M2_Forward, LOW);
  analogWrite(M2_Backward, LOW);
}
 
void tourner_droite_avancer() {
  digitalWrite(M1_Forward, LOW);
  digitalWrite(M1_Backward, LOW);
  analogWrite(M2_Forward, speed_Forward);
  digitalWrite(M2_Backward, LOW);
}
 
 
void motorTest() {
  Serial.println("Testing motors...");
 
  Serial.println("Moving forward...");
  analogWrite(GP8, 255);
  digitalWrite(GP9, LOW);
  analogWrite(GP10, 255);
  digitalWrite(GP11, LOW);
  delay(250);
 
  Serial.println("Moving backward...");
  digitalWrite(GP8, LOW);
  analogWrite(GP9, 255);
  digitalWrite(GP10, LOW);
  analogWrite(GP11, 255);
  delay(250);
 
  Serial.println("Stopping motors...");
  digitalWrite(GP8, LOW);
  digitalWrite(GP9, LOW);
  digitalWrite(GP10, LOW);
  digitalWrite(GP11, LOW);
  delay(1000);
 
  Serial.println("Motor test complete. Now ready for control.");
}
 
 
void setupWiFi() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  delay(1000);
  Serial.println(WiFi.localIP());
}
 
void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String command = String((char*)payload);
    Serial.print("Received: ");
    Serial.println(command);
    
    Systeme_Clavier(command);
    Systeme_automatique(command);
 
    DynamicJsonDocument doc(200);
    deserializeJson(doc, payload);
    String action = doc["action"];
 
    if (command == "x") {
      Serial.println("  braking = true;");
      braking = true;
      brake();
      lumiere_freiner();
 
    }
 
    if (action == "brake") {
      brakeActive = true;
      neutre();
      Serial.println("Brake activated!");
    }
    else if (action == "move") {
      joystickX = doc["angle"];
      joystickY = doc["distance"];
      speed = map(joystickY, 0, 100, 0, 1023);
 
      direction = joystickX;
      if (direction < 0) direction += 360;
      if (direction >= 360) direction -= 360;
 
      Serial.print("Received Joystick Data - Angle: ");
      Serial.print(direction);
      Serial.print("°, Distance: ");
      Serial.println(joystickY);
 
      int leftMotorSpeed = 0;
      int rightMotorSpeed = 0;
 
      if (joystickY == 0) {
        neutre();
         lumiere_freiner();
        return;
      }
      else if (direction >= 45 && direction < 135) {
        leftMotorSpeed = speed;
        rightMotorSpeed = speed;
      }
      else if (direction >= 135 && direction < 225) {
        float turnFactor = (direction - 135) / 90.0;
        leftMotorSpeed = speed;
        rightMotorSpeed = -speed * (1 - turnFactor);
      }
      else if (direction >= 225 && direction < 315) {
        leftMotorSpeed = -speed;
        rightMotorSpeed = -speed;
      }
      else {
        float turnFactor = (direction - 315) / 90.0;
        leftMotorSpeed = -speed * (1 - turnFactor);
        rightMotorSpeed = speed;
      }
     if (direction >= 225 && direction < 315) {
      reculerOn = true;
    } else {
      reculerOn = false;
      // immediately turn off rear LEDs
      strip1.setPixelColor(1, 0);
      strip1.setPixelColor(2, 0);
      strip1.show();
    }
      if (leftMotorSpeed >= 0 && rightMotorSpeed >= 0) {
        analogWrite(GP8, leftMotorSpeed);
        digitalWrite(GP9, LOW);
        analogWrite(GP10, rightMotorSpeed);
        digitalWrite(GP11, LOW);
      }
      else if (leftMotorSpeed < 0 && rightMotorSpeed < 0) {
        digitalWrite(GP8, LOW);
        analogWrite(GP9, abs(leftMotorSpeed));
        digitalWrite(GP10, LOW);
        analogWrite(GP11, abs(rightMotorSpeed));
      }
      else if (leftMotorSpeed >= 0 && rightMotorSpeed < 0) {
        analogWrite(GP8, leftMotorSpeed);
        digitalWrite(GP9, LOW);
        digitalWrite(GP10, LOW);
        analogWrite(GP11, abs(rightMotorSpeed));
      }
      else {
        digitalWrite(GP8, LOW);
        analogWrite(GP9, abs(leftMotorSpeed));
        analogWrite(GP10, rightMotorSpeed);
        digitalWrite(GP11, LOW);
      }
    }
      // 3) Clignotants (ajouter dans handleWebSocketEvent, après le bloc “move”)
  if (action == "light") {
  String cmd = doc["command"];  // “gauche”, “droite” ou “stop”
  if (cmd == "gauche" || cmd == "droite" || cmd == "stop" || cmd == "rear") {
    current_light_cmd = cmd;  // Update global state
  }
}
 
  // But first check for the raw movement strings:
  String msg = String((char*)payload);  // however you extract the text
  if (msg == "s") {
    // reverse command
    reculerOn = true;
  }
  else if (msg == "w" || msg == "l" || msg == "r" || msg == "x") {
    // any other movement stops reverse‐blinker
    reculerOn = false;
    // immediately turn LEDs off once
    strip1.setPixelColor(1, 0);
    strip1.setPixelColor(2, 0);
    strip1.show();
  }
 
 
}
 
  
  // then your existing handling for JSON commands...
}
 
 
 
 
  
 
 
 
 
void Systeme_Clavier(String command) {
  int speed2 = speed_Forward / 4;
 
  if (command == "") {
    speed_Forward = 0;
  }
  else if (command != previousCommand) {
    speed_Forward = 0;
  }
 
  if (command == "w") {
    avancer();
    accelerating = true;
    decelerating = false;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    avancer();
  }
  else if (command == "s") {
    reculer();
    lumiere_reculer();
    accelerating = false;
    decelerating = true;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    reculer();
     lumiere_reculer();
  }
  else if (command == "l") {
    tourner_gauche_avancer();
    accelerating = true;
    decelerating = false;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    tourner_gauche_avancer();
  }
  else if (command == "r") {
    tourner_droite_avancer();
    accelerating = true;
    decelerating = false;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    tourner_droite_avancer();
  }
  else if (command == "X") {
    neutre();
  
    lumiere_freiner();
 
  }
  else if (command == "E") {
    accelerating = true;
    decelerating = false;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    analogWrite(M1_Forward, speed2);
    digitalWrite(M1_Backward, LOW);
    analogWrite(M2_Forward, speed_Forward);
    digitalWrite(M2_Backward, LOW);
  }
  else if (command == "Q") {
    accelerating = true;
    decelerating = false;
    if (speed_Forward < 1023) {
      speed_Forward += 50;
      if (speed_Forward > 1023) speed_Forward = 1023;
    }
    analogWrite(M1_Forward, speed_Forward);
    digitalWrite(M1_Backward, LOW);
    analogWrite(M2_Forward, speed2);
    digitalWrite(M2_Backward, LOW);
  }
  previousCommand = command;
}
 
void gererTaxi() {
/*
    if(braking){
  brake();
  return;
} else {
 
*/
 
  if (!taxiActif) return; // Sort si le mode taxi n'est pas actif
 
   speed_Backward = 255;
   speed_Forward = 255;
 
 
 
  // On exécute toutes les étapes d'un coup, car avec delay() c'est bloquant et séquentiel
  switch (Etape) {
 
 
 
    case 1:
    reculer();
      Serial.println("Étape 1 : Reculer pour sortir du bureau actuel.");
      reculer();
      delay(2900); // Multiplier par 2
      neutre();
      delay(500); // Multiplier par 2
      Serial.println("Sorti du bureau.");
        Etape++;
      break;
 
    case 2:
      Serial.println("Étape 2 : Tourner vers l'autoroute.");
      if (departBureau <= 3) {
        Serial.println("Direction : Autoroute gauche.");
        tourner_gauche_avancer();
      } else {
        Serial.println("Direction : Autoroute droite.");
        tourner_droite_avancer();
      }
      delay(2900); // Multiplier par 2
      neutre();
      delay(500); // Multiplier par 2
      Serial.println("Orientation vers l'autoroute terminée.");
      Etape++;
      break;
 
    case 3:
      Serial.println("Étape 3 : Rejoindre l'autoroute.");
      if (departBureau <= 3) {
        tempsRejoindreAutoroute = departBureau * 2800 * 2; // Multiplier par 2
        Serial.print("Temps pour rejoindre l'autoroute gauche : ");
      } else {
        tempsRejoindreAutoroute = (6 - departBureau) * 2800 * 2; // Multiplier par 2
        Serial.print("Temps pour rejoindre l'autoroute droite : ");
      }
      Serial.println(tempsRejoindreAutoroute);
      avancer();
      delay(tempsRejoindreAutoroute); // Déjà multiplié par 2
      neutre();
      delay(500 ); // Multiplier par 2
      Serial.println("Rejoint l'autoroute.");
      Etape++;
      break;
 
    case 4:
      Serial.println("Étape 4 : Déplacement vertical vers la rangée cible.");
      ecartRangee = destRangee - departRangee;
      if (ecartRangee != 0) {
        if (departBureau <= 3) {
        if (ecartRangee < 0) {
          Serial.println("Direction : Descendre.");
          tourner_droite_avancer();
        } else {
          Serial.println("Direction : Monter.");
          tourner_gauche_avancer();
        }
      }else {
          if (ecartRangee > 0) {
          Serial.println("Direction : Descendre.");
          tourner_droite_avancer();
        } else {
          Serial.println("Direction : Monter.");
          tourner_gauche_avancer();
        }
        }
        delay(2900); // Multiplier par 2
        neutre();
        delay(500); // Multiplier par 2
 
        avancer();
        delay(9500 * abs(ecartRangee)); // Multiplier par 2
        neutre();
        delay(500); // Multiplier par 2
        Serial.println("Déplacement vertical terminé.");
 
        if (departBureau <= 3) {
          if (ecartRangee > 0) {
            Serial.println("Direction : Retour vers la gauche.");
            tourner_gauche_avancer();
          } else {
            Serial.println("Direction : Retour vers la droite.");
            tourner_droite_avancer();
          }
        } else {
          if (ecartRangee < 0) {
            Serial.println("Direction : Retour vers la gauche.");
            tourner_gauche_avancer();
          } else {
            Serial.println("Direction : Retour vers la droite.");
            tourner_droite_avancer();
          }
        }
        delay(3100); // Multiplier par 2
        neutre();
        delay(500 ); // Multiplier par 2
      }
      Etape++;
      break;
 
    case 5:
      Serial.println("Étape 5 : Déplacement horizontal jusqu'au bureau cible.");
      if (destBureau <= 3) {
        tempsVersBureau = destBureau * 6000; // Multiplier par 2
        Serial.print("Temps pour atteindre le bureau gauche : ");
      } else {
        tempsVersBureau = (6 - destBureau) * 6000; // Multiplier par 2
        Serial.print("Temps pour atteindre le bureau droit : ");
      }
      Serial.println(tempsVersBureau);
 
      avancer();
      delay(tempsVersBureau); // Déjà multiplié par 2
      neutre();
      delay(500); // Multiplier par 2
 
      Serial.println("Arrivée au bureau cible.");
      Etape++;
      break;
 
    case 6:
      Serial.println("Arrivé à destination !");
      taxiActif = false; // Terminé
      Etape = 1; // Remise à zéro
     
      break;
 
  }
//}
}
 
void Systeme_automatique(String command) {
/* if (command == "Vagabond") {
    mode_Vagabond();
  } else *//*if (command == "Explorateur") {
    Mode_Explorateur(command);
  } else*/ if (command.startsWith("TAXI:")) {
    Serial.print("toto1");
 
    departRangee = command.substring(5, 6).toInt();
    departBureau = command.substring(7, 8).toInt();
    destRangee = command.substring(9, 10).toInt();
    destBureau = command.substring(11, 12).toInt();
 
    Serial.println("Démarrage mode Taxi");
    Serial.print("Départ : Rangée "); Serial.print(departRangee); Serial.print(", Bureau "); Serial.println(departBureau);
    Serial.print("Destination : Rangée "); Serial.print(destRangee); Serial.print(", Bureau "); Serial.println(destBureau);
 
    taxiActif = true;
    Etape = 1;
  }
}
 
 
 
// Fonction pour allumer les phares avant
void lumiere_avant() {
   
    // Allumer ou éteindre les lumières avant en fonction de l'état
    if (etat_lumiere_avant == 0) {
        strip.setPixelColor(1, strip.Color(255, 255, 255)); // Met la lumière en blanc
        strip.setPixelColor(2, strip.Color(255, 255, 255));
        strip.show(); // Met à jour les lumières
    } else {
        strip.setPixelColor(1, strip.Color(0, 0, 0)); // Éteint la lumière
        strip.setPixelColor(2, strip.Color(0, 0, 0));
        strip.show(); // Met à jour les lumières
    }
}
 
// Fonction pour activer la marche arrière
void lumiere_reculer() {
   
    // Allumer les LED arrière en rouge si le bouton est pressé
    if (etat_recul == 0) {
        strip1.setPixelColor(1, strip1.Color(255, 0, 0)); // Allume les LED arrière en rouge
        strip1.setPixelColor(2, strip1.Color(255, 0, 0));
        strip1.show(); // Met à jour les lumières
    } else {
        strip1.setPixelColor(1, strip1.Color(0, 0, 0)); // Éteint les LED arrière
        strip1.setPixelColor(2, strip1.Color(0, 0, 0));
        strip1.show(); // Met à jour les lumières
    }
}
 
// Fonction pour activer les feux de freinage
void lumiere_freiner() {
   
        strip1.setPixelColor(1, strip1.Color(255, 0, 0)); // Allume les LED arrière en rouge
        strip1.setPixelColor(2, strip1.Color(255, 0, 0));
        strip1.show(); // Met à jour les lumières
 
}
 
// Fonction pour activer le clignotant gauche
void lumiere_gauche() {
  if (clignotantGaucheActif) {
    unsigned long now = millis();
    if (now - lastBlinkTime >= blinkDelay) {
      lastBlinkTime = now;
      blinkState = !blinkState;
 
      uint32_t color = blinkState ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
      strip.setPixelColor(0, color);
      strip1.setPixelColor(0, color);
      strip.show();
      strip1.show();
    }
  } else {
    // Ensure LED is off if not active
    strip.setPixelColor(0, 0);
    strip1.setPixelColor(0, 0);
    strip.show();
    strip1.show();
  }
}
 
void lumiere_droite() {
  if (clignotantDroiteActif) {
    unsigned long now = millis();
    if (now - lastBlinkTime >= blinkDelay) {
      lastBlinkTime = now;
      blinkState = !blinkState;
 
      uint32_t color = blinkState ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
      strip.setPixelColor(3, color);
      strip1.setPixelColor(3, color);
      strip.show();
      strip1.show();
    }
  } else {
    // Ensure LED is off if not active
    strip.setPixelColor(3, 0);
    strip1.setPixelColor(3, 0);
    strip.show();
    strip1.show();
  }
}
 
 
// Fonction pour éteindre toutes les lumières
void lumiere_eteindre() {
    strip.setPixelColor(0, strip.Color(0, 0, 0)); // Éteindre LED gauche
    strip1.setPixelColor(0, strip1.Color(0, 0, 0)); // Éteindre LED gauche sur strip1
    strip.setPixelColor(1, strip.Color(0, 0, 0)); // Éteindre LED avant
    strip.setPixelColor(2, strip.Color(0, 0, 0)); // Éteindre LED avant
    strip1.setPixelColor(1, strip1.Color(0, 0, 0)); // Éteindre LED arrière
    strip1.setPixelColor(2, strip1.Color(0, 0, 0)); // Éteindre LED arrière
    strip.setPixelColor(3, strip.Color(0, 0, 0)); // Éteindre LED droite
    strip1.setPixelColor(3, strip1.Color(0, 0, 0)); // Éteindre LED droite sur strip1
}
 
// Fonction pour gérer la luminosité des lumières en fonction des commandes
void gestion_luminosite(String command) {
 
    lumiere_eteindre(); // Éteindre toutes les lumières au début
 
    // Gestion des commandes d'assistance
    if (command == "assister") {
        if (command == "Phare") { // Commande pour activer les phares
         lumiere_avant(); // Allumer les phares
        }  
        if (command == "Reculer") { // Commande pour activer les feux de recul
            lumiere_freiner(); // Activer les feux de freinage
        }  
        if (command == "Droite") { // Commande pour activer le clignotant droit
   
            lumiere_droite(); // Activer les feux de freinage
        }
        if (command == "Gauche") { // Commande pour activer le clignotant gauche
            lumiere_gauche(); // Activer les feux de freinage
        }
        if (command == "Stop") { // Commande pour arrêter
            lumiere_freiner(); // Activer les feux de freinage
        }
 
    } else if (command == "manuel") { // Gestion des commandes manuelles
        if (command == "Phare") { // Commande pour activer les phares
          if (etat_lumiere_avant == 0) { // Si les phares ne sont pas allumés
                lumiere_avant(); // Allumer les phares
                etat_lumiere_avant++; // Mettre à jour l'état
            } else {
                lumiere_eteindre(); // Éteindre les phares
                etat_lumiere_avant = 0; // Réinitialiser l'état
            }
        }  
        if (command == "Reculer") { // Commande pour activer les feux de recul
            lumiere_freiner(); // Activer les feux de freinage
        }  
        if (command == "Clignotant_Droit") { // Commande pour activer le clignotant droit
            if (etat_Clg_Droite == 0) { // Si le clignotant droit n'est pas allumé
                lumiere_droite(); // Allumer le clignotant droit
                etat_Clg_Droite++; // Mettre à jour l'état
            } else {
                lumiere_eteindre(); // Éteindre le clignotant droit
                etat_Clg_Droite = 0; // Réinitialiser l'état
            }
        }  
        if (command == "Clignotant_Gauche") { // Commande pour activer le clignotant gauche
            if (etat_Clg_Gauche == 0) { // Si le clignotant gauche n'est pas allumé
                lumiere_gauche(); // Allumer le clignotant gauche
                etat_Clg_Gauche++; // Mettre à jour l'état
            } else {
                lumiere_eteindre(); // Éteindre le clignotant gauche
                etat_Clg_Gauche = 0; // Réinitialiser l'état
            }
        }
        if (command == "Stop") { // Commande pour arrêter
            lumiere_freiner(); // Activer les feux de freinage
        }
    }
}

// === Fonctions pour l'encodeur ===
void updateEncoderLeft() {
  int currentA = digitalRead(pinA_L);
  if (currentA != lastA_L) {
    if (digitalRead(pinB_L) != currentA)
      pos_left++;
    else
      pos_left--;
    lastA_L = currentA;
  }
}
 
void updateEncoderRight() {
  int currentA = digitalRead(pinA_R);
  if (currentA != lastA_R) {
    if (digitalRead(pinB_R) != currentA)
      pos_right++;
    else
      pos_right--;
    lastA_R = currentA;
  }
}
 
// === Connexion MQTT ===
void reconnectMQTT() {
Serial.print("Tentative de connexion MQTT... ");
if (client.connect("PicoW_Encoder")) {
  Serial.println("Connecté !");
  client.subscribe("robot/commands");
} else {
  Serial.print("Échec, code: ");
  Serial.print(client.state());
  Serial.println(" → nouvelle tentative dans 5s");
  delay(5000);
}
}
 
// === Setup complet à appeler dans setup() ===
void setupRobotMQTT() {
/*while (!Serial);  // Wait for Serial Monitor (optional)
Serial.println("Starting...");
  // Connexion Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connexion Wi-Fi en cours");
while (WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(500);
}
Serial.println("\nConnecté !");
Serial.print("IP locale : ");
Serial.println(WiFi.localIP());*/
 
  // Initialisation MQTT
  client.setServer(mqtt_server, 1883);
  reconnectMQTT();
 
  // Pins encodeurs
  pinMode(pinA_L, INPUT_PULLUP);
  pinMode(pinB_L, INPUT_PULLUP);
  pinMode(pinA_R, INPUT_PULLUP);
  pinMode(pinB_R, INPUT_PULLUP);
 
  lastA_L = digitalRead(pinA_L);
  lastA_R = digitalRead(pinA_R);
 
  attachInterrupt(digitalPinToInterrupt(pinA_L), updateEncoderLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinA_R), updateEncoderRight, CHANGE);
}
 
// === Loop complet à appeler dans loop() ===
void loopRobotMQTT() {
  if (!client.connected()) reconnectMQTT();
  client.loop();
 
  // Publication des données toutes les 500ms
  unsigned long now = millis();
  if (now - lastPublish > 500) {
    lastPublish = now;
 
    ArduinoJson::StaticJsonDocument<128> doc;
    doc["left_speed"] = pos_left;
    doc["right_speed"] = pos_right;
    //doc["temperature"] = tempSensorValue;  
    //doc["distance"] = distanceSensorValue;  
 
    char buffer[128];
    size_t n = serializeJson(doc, buffer);
    client.publish("robot/data", buffer, n);
  }
  Serial.println("Publishing...");
Serial.print("Left pos: "); Serial.print(pos_left);
Serial.print(" | Right pos: "); Serial.println(pos_right);
}
 
 
 
 
void setup() {
  Serial.begin(115200);
  setupWiFi();
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
  server.begin();
  motorTest();
 setupRobotMQTT();
  strip.begin(); // Initialise la bande de LED strip
  strip.show();// Met à jour les LED
  strip.setBrightness(BRIGHTNESS);// Définit la luminosité globale des LED
 
  strip1.begin();
  strip1.show();
  strip1.setBrightness(BRIGHTNESS);
  
 
}
 
bool Detection() {
  distance_cm = mySensor.distance();
  Serial.print("Distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  
  if (distance_cm < 9) {
    Serial.println("ARRET");
    return true;
    neutre();
  } else {
    return false;
  }
  delay(500);
}
 
// Assuming you have set up WebSocket, server, and other components
 
void loop() {
  loopRobotMQTT();
  webSocket.loop();
  server.handleClient();
  
  if(braking){
    lumiere_freiner();
    delay(400);
  }
  // If braking is not active, continue checking and managing taxi mode
  if (!braking) {
    gererTaxi();  // Handle taxi mode steps
  }
 
  // ---- reverse‐blinker handler ----
  if (reculerOn) {
    unsigned long now = millis();
    if (now - lastReculerBlink >= blinkDelay) {
      lastReculerBlink = now;
      reculerState = !reculerState;
      uint32_t c = reculerState
        ? strip1.Color(255, 0, 0)  // RED on
        : strip1.Color(0, 0, 0);   // OFF
      strip1.setPixelColor(1, c);
      strip1.setPixelColor(2, c);
      strip1.show();
    }
  }
 
 
 
// Update clignotant state flags based on command
if (current_light_cmd == "gauche") {
  clignotantGaucheActif = true;
  clignotantDroiteActif = false;
  lumiere_gauche();
}
else if (current_light_cmd == "droite") {
  clignotantDroiteActif = true;
  clignotantGaucheActif = false;
  lumiere_droite();
} else if (current_light_cmd == "rear") {
  lumiere_reculer();
}
else {
  clignotantGaucheActif = false;
  clignotantDroiteActif = false;
  lumiere_gauche();  // ensures they're both off
  lumiere_droite();
}

}
 