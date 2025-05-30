#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_crc.h>
#include "pages.h"

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

WebServer server(80);
Servo myservo;
Preferences preferences;

int servopin = 12;
int green = 27;
int red = 26;
bool doorState = false;

// ----------- FUNÇÕES ----------

String gerarIDUtilizador(const String& email) {
  uint32_t crc = esp_crc32_le(0, (const uint8_t*)email.c_str(), email.length());
  char id[9];
  sprintf(id, "%08X", crc);  // 8 caracteres HEX
  String resultado = String(id);

  Serial.print("ID gerado para ");
  Serial.print(email);
  Serial.print(" -> ");
  Serial.println(resultado);

  return resultado;
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


// Decodificação Base64 (manual)
String decodeBase64(const String& input) {
  const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int in_len = input.length();
  int i = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  String ret;

  while (in_len-- && input[in_] != '=' && isalnum(input[in_]) || input[in_] == '+' || input[in_] == '/') {
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

// Envia a página principal protegida
void sendHtml() {
  String response = MainPage;
  response.replace("green_TEXT", doorState ? "Aberto" : "Fechado");
  server.send(200, "text/html", response);
}

// ---------- SETUP & LOOP ----------

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  myservo.attach(servopin);

  server.on("/", []() { server.send(200, "text/html", Menu); });
  server.on("/registar", HTTP_GET, []() { server.send(200, "text/html", RegisterPage); });
  server.on("/login", []() { server.send(200, "text/html", LoginPage); });

  // Registo
  server.on("/registar", HTTP_POST, []() {
    if (!server.hasArg("plain")) return server.send(400, "text/plain", "Dados inválidos");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    guardarUtilizadorKV(doc["nome"], doc["email"], doc["password"], doc["CPassword"]);
  });

  // Login
  server.on("/login", HTTP_POST, []() {
    if (!server.hasArg("plain")) return server.send(400, "text/plain", "Dados inválidos");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    if (verificarLoginKV(doc["email"], doc["password"])) {
      server.send(200, "text/plain", "Login com sucesso!");
    } else {
      server.send(403, "text/plain", "Credenciais inválidas.");
    }
  });

  // Rota protegida
  server.on(UriBraces("/toggle/{}"), []() {
    String authHeader = server.header("Authorization");
    if (!authHeader.startsWith("Basic ")) return server.requestAuthentication();

    String encoded = authHeader.substring(6);
    String decoded = decodeBase64(encoded);
    int sep = decoded.indexOf(':');
    if (sep == -1) return server.requestAuthentication();

    String email = decoded.substring(0, sep);
    String password = decoded.substring(sep + 1);

    if (!verificarLoginKV(email, password)) return server.requestAuthentication();

    // Toggle
    doorState = !doorState;
    digitalWrite(green, doorState);
    digitalWrite(red, !doorState);
    myservo.write(doorState ? 90 : 180);
    sendHtml();
  });

  server.on("/main", sendHtml);

  server.begin();
  Serial.println("HTTP server iniciado.");
}

void loop() {
  server.handleClient();
}
