#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_crc.h>
#include <WiFiManager.h>
#include "pages.h"
#include <PubSubClient.h>

// #define WIFI_SSID "Joaquim-Colibri"
// #define WIFI_PASSWORD "arrozdepato"
// #define WIFI_CHANNEL 6

#define MQTT_BROKER "192.168.170.5"
#define MQTT_PORT 1883
#define MQTT_TOPIC "ipsantarem/esp32/porta"



WebServer server(80);
Servo myservo;
Preferences preferences;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int servopin = 12;
int green = 27;
int red = 26;
bool doorState = false;

// ----------- FUNÇÕES ----------

void reconnectMQTT() {
  static bool estavaLigado = false;

  if (!mqttClient.connected()) {
    if (estavaLigado) {
      Serial.println("Desligado do MQTT. A tentar reconectar...");
      estavaLigado = false;
    }

    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Ligado ao MQTT!");
      estavaLigado = true;
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" nova tentativa em 5 segundos");
      delay(5000);
    }
  }
}


String autenticarRequisicaoEmail() {
  String authHeader = server.header("Authorization");
  if (!authHeader.startsWith("Basic ")) return "";

  String encoded = authHeader.substring(6);
  String decoded = decodeBase64(encoded);
  int sep = decoded.indexOf(':');
  if (sep == -1) return "";

  String email = decoded.substring(0, sep);
  String password = decoded.substring(sep + 1);

  if (!verificarLoginKV(email, password)) return "";

  return email;
}


// Gerar ID com base no email
String gerarIDUtilizador(const String& email) {
  uint32_t crc = esp_crc32_le(0, (const uint8_t*)email.c_str(), email.length());
  char id[9];
  sprintf(id, "%08X", crc);
  return String(id);
}

// Registo (chave-valor)
void guardarUtilizadorKV(const String& nome, const String& email, const String& password, const String& CPassword) {
  if (password != CPassword) {
    server.send(400, "text/plain", "Erro nas passwords!");
    return;
  }

  String userID = gerarIDUtilizador(email);
  String keyNome = userID + "_n";
  String keyPass = userID + "_p";

  preferences.begin("users", false);

  if (preferences.isKey(keyPass.c_str())) {
    preferences.end();
    server.send(400, "text/plain", "Utilizador já existe!");
    return;
  }

  preferences.putString(keyNome.c_str(), nome);
  preferences.putString(keyPass.c_str(), password);

  preferences.end();
  server.send(200, "text/plain", "Utilizador registado com sucesso!");
}

// Verificação do login
bool verificarLoginKV(const String& email, const String& pass) {
  String userID = gerarIDUtilizador(email);
  String keyPass = userID + "_p";

  preferences.begin("users", true);
  String guardada = preferences.getString(keyPass.c_str(), "");
  preferences.end();

  return (guardada == pass);
}

// Decodificação Base64
String decodeBase64(const String& input) {
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int in_len = input.length();
  int i = 0, in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  String ret;

  while (in_len-- && input[in_] != '=' && (isalnum(input[in_]) || input[in_] == '+' || input[in_] == '/')) {
    char_array_4[i++] = input[in_]; in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++) char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;
      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
      for (i = 0; i < 3; i++) ret += (char)char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (int j = i; j < 4; j++) char_array_4[j] = 0;
    for (int j = 0; j < 4; j++) char_array_4[j] = strchr(base64_chars, char_array_4[j]) - base64_chars;
    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    for (int j = 0; j < i - 1; j++) ret += (char)char_array_3[j];
  }

  return ret;
}

// Autenticação via header Basic
bool autenticarRequisicao() {
  String authHeader = server.header("Authorization");
  if (!authHeader.startsWith("Basic ")) return false;

  String encoded = authHeader.substring(6);
  String decoded = decodeBase64(encoded);
  int sep = decoded.indexOf(':');
  if (sep == -1) return false;

  String email = decoded.substring(0, sep);
  String password = decoded.substring(sep + 1);
  return verificarLoginKV(email, password);
}

// Envia a página principal
void sendHtml() {

  if (!autenticarRequisicao()) {
    return server.requestAuthentication();
  }

  String response = MainPage;
  response.replace("green_TEXT", doorState ? "Aberto" : "Fechado");
  server.send(200, "text/html", response);
}

// ---------- SETUP & LOOP ----------

void setup() {
  WiFiManager wm;

  bool success = wm.autoConnect("ESP32-Config");

  if (!success) {
    Serial.println("Falha ao conectar, a reiniciar...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado! IP: " + WiFi.localIP().toString());


  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  myservo.attach(servopin);

  server.on("/", []() {
    if (!autenticarRequisicao()) return server.requestAuthentication();
    server.send(200, "text/html", Menu);
  });

  server.on("/registar", HTTP_GET, []() { server.send(200, "text/html", RegisterPage); });
  server.on("/login", []() { server.send(200, "text/html", LoginPage); });

  // POST Registo
  server.on("/registar", HTTP_POST, []() {
    if (!server.hasArg("plain")) return server.send(400, "text/plain", "Dados inválidos");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    guardarUtilizadorKV(doc["nome"], doc["email"], doc["password"], doc["CPassword"]);
  });


  server.on("/logout", []() {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"ESP Logout\"");
    server.send(401, "text/plain", "Sessão terminada. Recarrega e faz login novamente.");
  });



  // Toggle porta (requer autenticação)
  server.on(UriBraces("/toggle/{}"), []() {

    String email = autenticarRequisicaoEmail();
    if (email == "") return server.requestAuthentication();

    if (!autenticarRequisicao()) return server.requestAuthentication();

    doorState = !doorState;
    digitalWrite(green, doorState);
    digitalWrite(red, !doorState);
    myservo.write(doorState ? 90 : 180);
    String msg = email + (doorState ? " destrancou a porta" : " trancou a porta");
    mqttClient.publish(MQTT_TOPIC, msg.c_str());
    sendHtml();
  });

  server.on("/main", sendHtml);

  server.begin();
  Serial.println("HTTP server iniciado.");
}
void loop() {
  server.handleClient();

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();

  delay(10);  // <- pausa mínima para evitar WDT reset
}


