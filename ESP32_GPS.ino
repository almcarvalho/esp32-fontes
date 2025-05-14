#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>

// PINS
const int led_placa = 2;
//Definicao dos pinos Serial com o GPS
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600 //Baud Rate também

// CONFIGURAÇÕES
const String serverUrl = "https://rastreador-f9849afda432.herokuapp.com";

// Variaveis globais do GPS
double latitude = -8.141566;
double longitude = -34.903734;

// WiFiManager
WiFiManager wifiManager;

//  Define o barramento SErial que vai ser usado: 2
HardwareSerial gpsSerial(2);

// Funções
void setupWiFi();
void notificar();
void localizar();
void split(String texto);

// Variaveis para implementar millis() ao invés de loop()
unsigned long ultimaLocalizacao = 0;
const unsigned long intervaloLocalizacao = 10000;  // 10 segundos
bool precisaNotificar = false;



void setup() {
  pinMode(led_placa, OUTPUT);
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Comunicacao Serial com GPS OK");

  setupWiFi();
  delay(500);
  localizar();
  delay(1000);
  notificar();
}

void loop() {
  unsigned long agora = millis();

  // Checa se passou 10 segundos
  if (agora - ultimaLocalizacao >= intervaloLocalizacao) {
    localizar();
    if (latitude != 0.0 && longitude != 0.0) precisaNotificar = true;
    ultimaLocalizacao = agora;
  }

  // Envia notificação se necessário
  if (precisaNotificar) {
    notificar();
    precisaNotificar = false;
  }
}

void setupWiFi() {
  wifiManager.setConfigPortalTimeout(180); // timeout do portal em segundos

  Serial.println("Aguardando conexão Wi-Fi...");
  if (!wifiManager.autoConnect("ALARMEUP", "1234567890")) {
    Serial.println("Falha ao conectar. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado na rede Wifi.");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

void notificar() {
  HTTPClient https;
  Serial.println("[HTTPS] begin...");

  if (https.begin(serverUrl + "/location")) {
    https.addHeader("Content-Type", "application/json");

    // Variáveis de latitude e longitude
    //float latitude = -8.141566;
    //float longitude = -34.903734;

    String jsonPayload = "{\"latitude\": " + String(latitude, 6) + ", \"longitude\": " + String(longitude, 6) + "}";

    int httpCode = https.POST(jsonPayload);

    if (httpCode == 200) {
      Serial.println("Notificação enviada com sucesso.");

      // Piscar o LED 5 vezes
      for (int i = 0; i < 5; i++) {
        digitalWrite(led_placa, HIGH);
        delay(200);
        digitalWrite(led_placa, LOW);
        delay(200);
      }
    } else {
      Serial.printf("Erro ao enviar notificação. Código HTTP: %d\n", httpCode);
    }

    https.end();
  } else {
    Serial.println("[HTTPS] Falha na conexão.");
  }
}

void split(String texto){
  int i1 = texto.indexOf(',');
  String satelite = texto.substring(0, i1);

  int it;
  int ia;
  int i2;
  int i3;
  int i4;

  String latitudeDeg;
  String latitudeMin;
  String hemisferio;
  String longitudeDeg;
  String longitudeMin;
  String hemisLong;

  if(satelite == "GPGLL"){
    i2 = texto.indexOf(',', i1+1);
    i3 = texto.indexOf(',', i2+1);
    i4 = texto.indexOf(',', i3+1);

    latitudeDeg = texto.substring(i1 + 1, i1+3);
    latitudeMin = texto.substring(i1+4, i2);
    hemisferio = texto.substring(i2 + 1, i3);
    longitudeDeg = texto.substring(i3 + 1, i3+4);
    longitudeMin = texto.substring(i3+5, i4);
    hemisLong = texto.substring(i4 + 1, i4+2);
  }
  else if(satelite == "GPGGA"){
    it = texto.indexOf(',', i1+1);
    i2 = texto.indexOf(',', it+1);
    i3 = texto.indexOf(',', i2+1);
    i4 = texto.indexOf(',',i3+1);
    
    latitudeDeg = texto.substring(it + 1, it+3);
    latitudeMin = texto.substring(it+4, i2);
    hemisferio = texto.substring(i2 + 1, i3);
    longitudeDeg = texto.substring(i3 + 1, i3+4);
    longitudeMin = texto.substring(i3+5, i4);
    hemisLong = texto.substring(i4 + 1, i4+2);
  }

  else if(satelite == "GPRMC"){
    it = texto.indexOf(',', i1+1);
    ia = texto.indexOf(',', it+1);
    i2 = texto.indexOf(',', ia+1);
    i3 = texto.indexOf(',', i2+1);
    i4 = texto.indexOf(',', i3+1);
    
    latitudeDeg = texto.substring(ia + 1, ia+3);
    latitudeMin = texto.substring(ia+4, i2);
    hemisferio = texto.substring(i2 + 1, i3);
    longitudeDeg = texto.substring(i3 + 1, i3+4);
    longitudeMin = texto.substring(i3+5, i4);
    hemisLong = texto.substring(i4 + 1, i4+2);
  }

  /* else{
    latitudeDeg = "00";
    latitudeMin = "00.00000";
    hemisferio = "X";
    longitudeDeg = "00";
    latitudeMin = "00.00000";
    hemisLong = "Y";
  } */

  Serial.println(satelite);
  Serial.println(latitudeDeg);
  Serial.println(hemisferio);
  Serial.println(longitudeDeg);
  Serial.println(hemisLong);

  latitude = latitudeDeg.toFloat()+(latitudeMin.toFloat()/60.00);
  longitude = longitudeDeg.toFloat()+(longitudeMin.toFloat()/60.00);

  if(hemisLong == "W"){
    longitude*=-1;
  }
  if(hemisferio == "S"){
    latitude*=-1;
  }
}

void localizar(){
  String strGPS = gpsSerial.readStringUntil('$');
  Serial.println(strGPS);
  Serial.println("Split: ");
  split(strGPS);
  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("longitude: ");
  Serial.println(longitude, 6);
  
  delay(1000);
  Serial.println("-------------------------------");
}