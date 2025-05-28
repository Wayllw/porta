// Código completo com registo/login usando EEPROM

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <pages.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6
#define EEPROM_SIZE 1024

WebServer server(80);
Servo myservo;
int servopin = 12;
int green = 27;
int red = 26;
bool doorState = false;
bool loginOK = false;
String utilizadorAtual = "";

// ----- Funções EEPROM -----

void guardarUtilizadorEEPROM(const String& nome, const String& email, const String& password, const String& CPassword) {
  String dadosExistentes;
  for (int i = 0; i < EEPROM_SIZE; i++) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    dadosExistentes += c;
  }

  if (dadosExistentes.length() == 0) dadosExistentes = "[]";

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, dadosExistentes);
  if (password == CPassword){
    JsonObject novoUser = doc.createNestedObject();
    novoUser["nome"] = nome;
    novoUser["email"] = email;
    novoUser["password"] = password;

    String novoJSON;
    serializeJson(doc, novoJSON);

    for (int i = 0; i < novoJSON.length(); i++) EEPROM.write(i, novoJSON[i]);
    EEPROM.write(novoJSON.length(), '\0');
    EEPROM.commit();
    server.send(200, "text/plain", "Utilizador registado com sucesso!");
  } else {
    server.send(400, "text/plain", "Erro nas passwords!");
  }
}

bool verificarLoginEEPROM(const String& email, const String& pass) {
  String dados;
  for (int i = 0; i < EEPROM_SIZE; i++) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    dados += c;
  }

  Serial.println("Dados lidos da EEPROM:");
  Serial.println(dados);  // <-- Aqui é onde vês o conteúdo JSON

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, dados);
  if (err) return false;

  for (JsonObject user : doc.as<JsonArray>()) {
    if (user["email"] == email && user["password"] == pass) return true;
  }

  return false;
}

// ----- Código principal -----

void sendHtml() {
  if (utilizadorAtual == "") {
    return server.send(403, "text/plain", "Acesso negado");
  }
  String response = MainPage;
  response.replace("green_TEXT", doorState ? "Aberto" : "Fechado");
  server.send(200, "text/html", response);
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  myservo.attach(servopin);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) { delay(100); }
  server.on("/", [](){ server.send(200, "text/html", Menu); });
  server.on("/login", []() { server.send(200, "text/html", LoginPage); });
  server.on("/registar", HTTP_GET, []() { server.send(200, "text/html", RegisterPage); });
  server.on("/main", sendHtml);

  server.on("/registar", HTTP_POST, []() {
    if (!server.hasArg("plain")) return server.send(400, "text/plain", "Dados inválidos");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    guardarUtilizadorEEPROM(doc["nome"], doc["email"], doc["password"], doc["CPassword"]);
  });

  

  server.on("/login", HTTP_POST, []() {
    if (!server.hasArg("plain")) return server.send(400, "text/plain", "Dados inválidos");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    if (verificarLoginEEPROM(doc["email"], doc["password"])) {
      utilizadorAtual = doc["email"].as<String>();
      server.send(200, "text/plain", "Login com sucesso!");
    } else {
      server.send(403, "text/plain", "Credenciais inválidas.");
    }
  });

  server.on(UriBraces("/toggle/{}"), []() {
    
    doorState = !doorState;
    digitalWrite(green, doorState);
    digitalWrite(red, !doorState);
    myservo.write(doorState ? 90 : 180);
    sendHtml();
  });

  server.begin();
  Serial.println("HTTP server started (http://localhost:8180)");
}

void loop() {
  server.handleClient();
}
