#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "credentials.h"
WiFiClientSecure httpClient;
/* Function to connect to WiFi */
void connectToWiFi();
/* Function to make HTTP request*/
bool sendHTTPRequest(String prompt, String *result);
/* Function to pass user prompt and send it to OpenAI API*/
String getGptResponse(String prompt, bool parseMsg = true);

void setup()
{
  Serial.begin(115200);
  connectToWiFi();
}

void loop()
{
  if (Serial.available() > 0)
  {
    String prompt = Serial.readStringUntil('\n');
    prompt.trim();
    Serial.print("ESP32 > ");
    Serial.println(prompt);
    String response = getGptResponse(prompt);
    Serial.println("ChatGPT > " + response);
    delay(3000);
  }
  delay(10);
}

void connectToWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conexión WiFi exitosa");
}

bool sendHTTPRequest(String prompt, String *result)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("ERROR: Dispositivo no connectado a WiFi");
    return false;
  }

  // Connect to OpenAI API URL
  httpClient.setInsecure();
  if (!httpClient.connect(host, httpsPort))
  {
    Serial.println("Error al conectar con API OpenAI");
    return false;
  }
  // Build Payload
  String payload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"" + prompt + "\"}]}";
  Serial.println(payload);

  // Build HTTP Request
  String request = "POST /v1/chat/completions HTTP/1.1\r\n";
  request += "Host: " + String(host) + "\r\n";
  request += "Authorization: Bearer " + String(api_key) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(payload.length()) + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n" + payload + "\r\n";
  // Send HTTP Request
  httpClient.print(request);

  // Get Response
  String response = "";
  while (httpClient.connected())
  {
    if (httpClient.available())
    {
      response += httpClient.readStringUntil('\n');
      response += String("\r\n");
    }
  }
  httpClient.stop();

  // Parse HTTP Response Code
  int responseCode = 0;
  if (response.indexOf(" ") != -1)
  {                                                                                                  // If the first space is found
    responseCode = response.substring(response.indexOf(" ") + 1, response.indexOf(" ") + 4).toInt(); // Get the characters following the first space and convert to integer
  }

  if (responseCode != 200)
  {
    Serial.println("La petición ha fallado. Info:" + String(response));
    return false;
  }

  // Get JSON Body
  int start = response.indexOf("{");
  int end = response.lastIndexOf("}");
  String jsonBody = response.substring(start, end + 1);

  if (jsonBody.length() > 0)
  {
    *result = jsonBody;
    return true;
  }
  Serial.println("Error: no se ha podido leer la información");
  return false;
}

String getGptResponse(String prompt, bool parseMsg)
{
  String resultStr;
  bool result = sendHTTPRequest(prompt, &resultStr);
  if (!result) return "Error : sendHTTPRequest";
  if (!parseMsg) return resultStr;
  DynamicJsonDocument doc(resultStr.length() + 200);
  DeserializationError error = deserializeJson(doc, resultStr.c_str());
  if (error)
  {
    return "[ERR] deserializeJson() failed: " + String(error.f_str());
  }
  const char *_content = doc["choices"][0]["message"]["content"];
  return String(_content);
}
