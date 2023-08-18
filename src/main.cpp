#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "credentials.h"
HTTPClient http;
String assistantMsg = "actúa como un asistente amable";
void connectToWiFi();
String getChatGptResponse(String userMessage);
void setup() {
    Serial.begin(115200);
    connectToWiFi();
}

void loop() {
    if(Serial.available() > 0)
    {
      String prompt = Serial.readStringUntil('\n');
      prompt.trim();
      Serial.print("ESP32 > ");
      Serial.println(prompt);
      String response = getChatGptResponse(prompt);
      Serial.println("ChatGPT > " + response);
      delay(3000);
    }
    delay(10);
}

void connectToWiFi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    Serial.println("Conexión WiFi exitosa");
}

String getChatGptResponse(String userMessage) {
    if (WiFi.status() != WL_CONNECTED) return "ERROR: Dispositivo no connectado a WiFi";
    //Connect to OpenAI API URL
    if (!http.begin(apiURL)) {
        return "Error al conectar con API OpenAI";
    }
    //Build Payload
    /*
    Assistant messages store previous assistant responses, but can also be written by you to give examples of desired behavior.
    Including conversation history is important when user instructions refer to prior messages.
    */ 
    String assistantMsgJSON = "{\"role\": \"assistant\", \"content\": \"" + assistantMsg + "\"}";
    // String payload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [" + assistantMsgJSON + ",{\"role\": \"user\", \"content\": \"" + userMessage + "\"}]}";
    String payload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": ["+ assistantMsgJSON + ",{\"role\": \"user\", \"content\": \"" + userMessage + "\"}]}";
    Serial.println(payload);
    //Build HTTP Requests
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(api_key));
    //Get OpenAI Response
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println(response);
        const uint16_t responseLength = response.length();
        // Parse JSON response
        DynamicJsonDocument jsonDoc(responseLength + 200);
        deserializeJson(jsonDoc, response);
        String outputText = jsonDoc["choices"][0]["message"]["content"];
        outputText.remove(outputText.indexOf('\n'));
        assistantMsg = outputText;
        return outputText;
    }
    Serial.println(http.getString());
    return String("La petición ha fallado. Code: %d" + String(httpResponseCode));
}
