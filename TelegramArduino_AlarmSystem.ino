#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <UniversalTelegramBot.h>

// ===== CONFIGURAZIONE WIFI =====
#define WIFI_SSID "SSD-WIFI"
#define WIFI_PASSWORD "PSWD WIFI"

// ===== CONFIGURAZIONE TELEGRAM =====
#define BOT_API_TOKEN "BOT_TOKEN"

// ===== PIN HC-SR04 =====
const int TRIG_PIN = 2;   // Trigger ultrasuoni
const int ECHO_PIN = 3;   // Echo ultrasuoni
const int LED_PIN = 9;    // LED di stato

// ===== VARIABILI =====
WiFiSSLClient securedClient;
UniversalTelegramBot bot(BOT_API_TOKEN, securedClient);
String chat_id = "";           // chat ID dell'utente
int message_id = -1;
String from_name = "Unknown";

bool sistemaArmato = false;   // stato del sistema antifurto

// Timer per controllo Telegram
const unsigned long interval = 1000;
unsigned long last_call = 0;

// Tastiera inline
String inlineKeyboardJson = "";

// ===== FUNZIONI =====
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms ≈ 5m
  long distance = duration * 0.034 / 2;           // cm
  return distance;
}

void updateInlineKeyboard() {
  inlineKeyboardJson =
    "["
      "[{ \"text\" : " + String((sistemaArmato ? "\"Disarma Sistema\"" : "\"Arma Sistema\"")) + ", \"callback_data\" : \"/toggleSystem\" }]"
    "]";
}

void toggleSystem() {
  sistemaArmato = !sistemaArmato;
  digitalWrite(LED_PIN, sistemaArmato ? HIGH : LOW);
  updateInlineKeyboard();
  bot.sendMessageWithInlineKeyboard(chat_id, "Sistema " + String(sistemaArmato ? "ARMATO ✅" : "DISARMATO ❌"), "Markdown", inlineKeyboardJson, message_id);
}

void handleMessages(int num_new_messages) {
  for (int i = 0; i < num_new_messages; i++) {
    message_id = bot.messages[i].message_id;
    chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Unknown";

    if (text == "/start") {
      updateInlineKeyboard();
      bot.sendMessageWithInlineKeyboard(chat_id, "Ciao " + (from_name == "Unknown" ? "" : from_name) + ", scegli un'opzione:", "Markdown", inlineKeyboardJson);
    } else if (text == "/toggleSystem") {
      toggleSystem();
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Connessione WiFi
  Serial.println("Connessione WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  securedClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connesso! IP: " + WiFi.localIP().toString());

  Serial.println("Bot Telegram pronto!");
  updateInlineKeyboard();
}

// ===== LOOP =====
void loop() {
  // Controllo messaggi Telegram
  if (millis() - last_call > interval) {
    int num_new_messages = bot.getUpdates(bot.last_message_received + 1);
    if (num_new_messages) handleMessages(num_new_messages);
    last_call = millis();
  }

  // Controllo ultrasuoni solo se sistema armato
  if (sistemaArmato) {
    long distanza = readDistanceCM();
    Serial.print("Distanza: ");
    Serial.print(distanza);
    Serial.println(" cm");

    if (distanza > 10 && distanza < 60) { // soglia: <100 cm → movimento
      bot.sendMessage(chat_id, "🚨 Movimento rilevato! Distanza: " + String(distanza) + " cm");
      delay(5000); // evita flood di notifiche
    }
  }
}
