#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "credentials.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

byte customBackslash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};

// The display is 16 chars long. However, we allow a buffer of 64
// characters so that writing does not wrap.
LiquidCrystal_I2C lcd(0x27, 64, 2);  // set the LCD address to 0x27 for a 64 chars and 2 line display

int show = 0;

const __FlashStringHelper *wifi_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD:
      return F("No shield");  // for compatibility with WiFi Shield library
    case WL_IDLE_STATUS:
      return F("Waiting");
    case WL_NO_SSID_AVAIL:
      return F("Bad network");
    case WL_SCAN_COMPLETED:
      return F("Scanned");
    case WL_CONNECTED:
      return F("Connected!");
    case WL_CONNECT_FAILED:
      return F("Failed");
    case WL_CONNECTION_LOST:
      return F("Lost connection");
    case WL_DISCONNECTED:
      return F("Disconnected");
  }
}

static char attempt_chars[4] = { '|', '/', '-', 7 };

void connect_to_wifi() {
  lcd.setCursor(0, 0);
  lcd.print(F("Connect to Wifi"));
  lcd.setCursor(0, 1);

  WiFi.begin(ssid, password);

  int attempts = 0;

  wl_status_t wifi_status;

  do {
    lcd.setCursor(0, 1);
    lcd.print(attempt_chars[attempts++]);
    attempts %= sizeof(attempt_chars);

    lcd.setCursor(2, 1);
    lcd.print("              ");
    lcd.setCursor(2, 1);
    wifi_status = WiFi.status();

    auto wifi_string = wifi_status_to_string(wifi_status);

    lcd.print(wifi_string);
    Serial.printf("[Wifi Status] %s\n", wifi_string);

    delay(200);
  } while (wifi_status != WL_CONNECTED);
}

const char GOOGLE_AUTH_SERVER[] = "https://oauth2.googleapis.com/device/code";

// this is the root certificate of the google authentication server.
const char CA_CERT_GOOGLE_AUTH_SERVER[] =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
  "CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
  "MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
  "MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
  "Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
  "A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
  "27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
  "Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
  "TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
  "qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
  "szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
  "Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
  "MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
  "wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
  "aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
  "VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
  "AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
  "FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
  "C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
  "QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
  "h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
  "7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
  "ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
  "MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
  "Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
  "6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
  "0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
  "2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
  "bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
  "-----END CERTIFICATE-----\n";

void oauth_login(char *device_code_dst, char *user_code_dst, int *expires_in_dst, char *verify_url_dst) {
restart:
  lcd.clear();
  lcd.home();
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("Google Auth...");

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    auto begin_code = http.begin(client, GOOGLE_AUTH_SERVER);
    Serial.print("[INIT Google Auth Handshake] Begin code = ");
    Serial.println(begin_code);

    //application/x-www-form-urlencoded
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int responseCode = http.POST(OAUTH_INIT_ENDPOINT_BODY);
    /////////////////////////////////////
    Serial.print("[INIT Google Auth Handshake] HTTP Response code = ");
    Serial.println(responseCode);
    if (responseCode != 200 && responseCode != 301) {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(responseCode).c_str());

      Serial.print(http.getString());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("See serial port"));

      Serial.println(F("[INIT Google Auth Handshake] This error renders the Calendar API inaccessible. Try updating the auth certificate."));

      http.end();

      // hang
      while (1) {}
    }


    String raw_json = http.getString();

    Serial.print("[INIT Google Auth Handshake] Auth Response (JSON) = ");
    Serial.println(raw_json);

    DynamicJsonDocument doc(2048);

    deserializeJson(doc, raw_json);

    const char *device_code = doc["device_code"];
    const char *user_code = doc["user_code"];
    int expires_in = doc["expires_in"];
    const char *verify_url = doc["verification_url"];

    Serial.print("[INIT Google Auth Handshake] Result:\n[INIT Google Auth Handshake] Device code = ");
    Serial.println(device_code);
    Serial.print("[INIT Google Auth Handshake] User code = ");
    Serial.println(user_code);
    Serial.print("[INIT Google Auth Handshake] Expires in = ");
    Serial.println(expires_in);
    Serial.print("[INIT Google Auth Handshake] URL = ");
    Serial.println(verify_url);

    strcpy(device_code_dst, device_code);
    strcpy(user_code_dst, user_code);
    strcpy(verify_url_dst, verify_url);
    *expires_in_dst = expires_in;

    http.end();
  } else {
    Serial.println("[INIT Google Auth Handshake] Disconnected");

    connect_to_wifi();

    goto restart;
  }
}

const char GOOGLE_POLL_SERVER[] = "https://oauth2.googleapis.com/token";

// TODO
void get_new_access_token(const char *refresh_token, char *access_token_dst) {
  Serial.println("Getting new access token");
restart:
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    int begin_code = http.begin(client, GOOGLE_POLL_SERVER);

    Serial.print("[@@] Begin code = ");
    Serial.println(begin_code);

    //application/x-www-form-urlencoded
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    char body[512];

    snprintf(body, sizeof(body), "grant_type=refresh_token&client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&refresh_token=%s", refresh_token);

    int responseCode = http.POST(body);

    Serial.print("Response code = ");
    Serial.println(responseCode);


  } else {
    Serial.println("Disconnected");

    connect_to_wifi();

    goto restart;
  }
}

void oauth_poll(const char *device_code, const char *user_code, int expires, const char *verify_url) {
  Serial.printf("[Google Auth Handshake Verify] Displaying code \"%s\"\n", user_code);
  lcd.clear();

  int leftSpacing = (16 - strlen(user_code)) / 2;

  lcd.setCursor(leftSpacing, 1);
  lcd.print(user_code);

  char *connection_url;

  unsigned long seconds_start = millis() / 1000;

  unsigned long seconds_expires = seconds_start + expires;

  if (strcmp(verify_url, "https://www.google.com/device") == 0) {
    connection_url = "bit.ly/dev-conn";
  } else {
    connection_url = "[!] Check Serial";
    Serial.printf("[Google Auth Handshake Verify] While Google's endpoint usually returns <https://www.google.com/device> for authentication, you must verify at <%s>. You're special!\n", verify_url);
  }

  // max message len = 12
  const char *messages[] = {
    "To sync your",
    "Google Accou",
    "nt, enter th",
    "is code at: ",
    connection_url  // except for the last entry, which can be 16
  };

  int message_len = 5;
  int message_idx = 0;

  WiFiClientSecure client;
  client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

  while (1) {
    unsigned long seconds_now = millis() / 1000;

    if (seconds_now > seconds_expires) {
      lcd.clear();
      lcd.home();
      lcd.print("Code expired!");
      lcd.setCursor(0, 1);
      lcd.print("Reset to retry");

      Serial.println("[Google Auth Handshake Verify] Timeout! Please reset the device to get a new code.");

      // hang
      while (1) {}
    }

    lcd.setCursor(0, 0);
    lcd.print("                ");

    lcd.setCursor(0, 0);

    if (message_idx != message_len - 1) {
      lcd.print(message_idx + 1);
      lcd.print("/");
      lcd.print(message_len);
      lcd.print(" ");
    }

    lcd.print(messages[message_idx++]);
    message_idx %= message_len;

    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    auto begin_code = http.begin(client, GOOGLE_POLL_SERVER);
    Serial.print("[Google Auth Handshake Verify] HTTP Begin code = ");
    Serial.println(begin_code);

    //application/x-www-form-urlencoded
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    char body[512];

    snprintf(body, sizeof(body), "grant_type=urn:ietf:params:oauth:grant-type:device_code&client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&device_code=%s", device_code);

    int responseCode = http.POST(body);

    Serial.print("[Google Auth Handshake Verify] Response code = ");

    if (responseCode == 428) {
      Serial.println("428 (User has not visited the link)");
      delay(5000);
      continue;
    } else {
      Serial.println(responseCode);
    }

    if (responseCode != 200 && responseCode != 301) {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(responseCode).c_str());

      if (responseCode == 400) {
        Serial.println(http.getString());
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("See serial port"));

      Serial.println(F("[Google Auth Handshake Verify] This error renders the Calendar API inaccessible. Try updating the auth certificate."));

      http.end();

      // hang
      while (1) {}
    }


    String raw_json = http.getString();

    Serial.print("[Google Auth Handshake Verify] Auth Response = ");
    Serial.println(raw_json);

    DynamicJsonDocument doc(2048);

    deserializeJson(doc, raw_json);

    const char *access_token = doc["access_token"];
    int expires_in = doc["expires_in"];
    const char *refresh_token = doc["refresh_token"];
    const char *scope = doc["scope"];
    const char *token_type = doc["token_type"];
    const char *id_token = doc["id_token"];

    Serial.print("[Google Auth Handshake Verify] Access token = ");
    Serial.println(access_token);
    Serial.print("[Google Auth Handshake Verify] Expires in = ");
    Serial.println(expires_in);
    Serial.print("[Google Auth Handshake Verify] Refresh token = ");
    Serial.println(refresh_token);
    Serial.print("[Google Auth Handshake Verify] Scope = ");
    Serial.println(scope);
    Serial.print("[Google Auth Handshake Verify] Token Type = ");
    Serial.println(token_type);
    Serial.print("[Google Auth Handshake Verify] ID token = ");
    Serial.println(id_token);

    lcd.clear();
    lcd.home();
    lcd.print("Your account is");
    lcd.setCursor(0, 1);
    lcd.print("properly linked!");

    delay(5000);

    http.end();

    return;
  }
}

void setup() {
  lcd.init();  // initialize the lcd
  lcd.clear();
  lcd.createChar(7, customBackslash);

  Serial.begin(9600);
  
  Serial.println(F(
    "\n\nInit debug port\n\n"
    "  ___ ___ ___ _______    ___      _             _          \n"
    " | __/ __| _ \__ /_  )  / __|__ _| |___ _ _  __| |__ _ _ _ \n"
    " | _|\__ \  _/|_ \/ /  | (__/ _` | / -_) ' \/ _` / _` | '_|\n"
    " |___|___/_| |___/___|  \___\__,_|_\___|_||_\__,_\__,_|_|  \n"
    "\nMade by Mateo Rodriguez in 2023\n\n"                                                       
    ));

  lcd.backlight();

  connect_to_wifi();

  delay(2000);

  lcd.clear();
  lcd.noDisplay();

  delay(200);

  lcd.setCursor(0, 0);
  lcd.print("Online with IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.print("[Setup] Online with IP: ");
  Serial.println(WiFi.localIP());
  lcd.display();

  delay(4000);

  char device_code[128];
  char user_code[16];
  char verify_url[41];
  int expires_in;

  Serial.println("[Setup] Starting Auth Flow with Google Servers");
  oauth_login(device_code, user_code, &expires_in, verify_url);
  Serial.println("[Setup] Confirming Auth Flow with Google Servers");
  oauth_poll(device_code, user_code, expires_in, verify_url);

  while (1) {}

  delay(4000);
}

int counter = 1;

void loop() {
  lcd.clear();
  lcd.home();

  wl_status_t wifi_status = WiFi.status();

  if (wifi_status != WL_CONNECTED) {
    lcd.print("Lost WiFi Signal");

    delay(4000);

    connect_to_wifi();

    lcd.setCursor(0, 0);
    lcd.print("Back online!");
    delay(2000);
    lcd.clear();
  }



  lcd.print("Lorem Ipsum");
  lcd.setCursor(0, 1);
  lcd.print(counter++);

  delay(10000);
}