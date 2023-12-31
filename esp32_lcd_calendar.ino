#include <stdarg.h>
#include <sntp.h>
#include <time.h>

#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "config.h"

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
const char OAUTH_INIT_ENDPOINT_BODY[] = "client_id=" CLIENT_ID "&scope=email%20profile%20https://www.googleapis.com/auth/calendar.readonly";


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
    Serial.print("[INIT Google Auth Handshake] HTTP Response code = ");

    if (WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(0, 0);
      lcd.print("Lost signal");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
      delay(2000);

      Serial.print("[INIT Google Auth Handshake] Lost signal, restarting polling...");

      connect_to_wifi();

      Serial.print("[INIT Google Auth Handshake] Back online, restarting polling...");

      http.end();

      goto restart;
    } else if (responseCode < 0) {
      Serial.printf("NET_ERR\n[HTTPS] POST... failed, error: %s. WILL RETRY\n", http.errorToString(responseCode).c_str());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("Retrying..."));

      http.end();

      goto restart;
    }

    Serial.println(responseCode);
    if (responseCode != 200 && responseCode != 301) {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(responseCode).c_str());

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

// https://developers.google.com/identity/protocols/oauth2#:~:text=Tokens%20can%20vary%20in%20size,Refresh%20tokens%3A%20512%20bytes
#define MAX_ACCESS_TOKEN_LEN 2050  // 2048 per docs, + 2 for null byte and rounding
#define MAX_REFRESH_TOKEN_LEN 520  // 512 per docs, + 8 for null byte and rounding

#define GOOGLE_POLL_SERVER "https://oauth2.googleapis.com/token"

bool checked_snprintf(char *str, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(str, size, format, args);
  va_end(args);

  Serial.printf("[Checked snPrintF] last byte = %d\n", static_cast<int>(str[size - 1]));

  return str[size - 1] == 0;
}

char access_token[MAX_ACCESS_TOKEN_LEN];
char refresh_token[MAX_REFRESH_TOKEN_LEN];
unsigned long auth_token_expires_in = 0;
unsigned long auth_token_init = 0;

void oauth_poll(const char *device_code, const char *user_code, int expires, const char *verify_url) {
  unsigned long secondsAtStart = millis() / 1000;
  unsigned long secondsWillExpire = secondsAtStart + expires;

restart:
  Serial.printf("[Google Auth Handshake Verify] Displaying code \"%s\"\n", user_code);
  lcd.clear();

  int leftSpacing = (16 - strlen(user_code)) / 2;

  lcd.setCursor(leftSpacing, 1);
  lcd.print(user_code);

  char *connection_url;

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
    unsigned long secondsNow = millis() / 1000;
    unsigned long timeLeft = secondsWillExpire - secondsNow;

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

    // TODO: I don't like the dependence on the heap--try looking at other options (snprintf, etc. for templating).
    // There's only one string + string concatenation, so maybe a simple strcpy/memcpy could suffice?
    String body = String("grant_type=urn:ietf:params:oauth:grant-type:device_code&client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&device_code=") + device_code;

    int responseCode = http.POST(body);

    Serial.print("[Google Auth Handshake Verify] Response code = ");

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("NET_ERR");
      lcd.setCursor(0, 0);
      lcd.print("Lost signal");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
      delay(2000);

      Serial.println("[Google Auth Handshake Verify] Lost signal, restarting polling...");

      connect_to_wifi();

      Serial.println("[Google Auth Handshake Verify] Back online, restarting polling...");

      http.end();

      goto restart;
    } else if (responseCode < 0) {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(responseCode).c_str());
      lcd.clear();
      lcd.home();
      lcd.print("Error polling");
      lcd.setCursor(0, 1);
      lcd.print("Will retry in 2s");
      delay(2000);
      http.end();
      goto restart;
    }

    if (responseCode == 400 && timeLeft <= 0) {
      lcd.clear();
      lcd.home();
      lcd.print("Code expired!");
      lcd.setCursor(0, 1);
      lcd.print("Reset to retry");

      Serial.println("[Google Auth Handshake Verify] Timeout! Please reset the device to get a new code.");

      // hang
      while (1) {}
    }

    if (responseCode == 428) {
      Serial.printf("428 (User has not visited the link, %u seconds remaining)\n", timeLeft);
      delay(5000);
      continue;
    } else {
      Serial.println(responseCode);
    }

    if (responseCode != 200 && responseCode != 301) {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(responseCode).c_str());

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

    auth_token_init = millis() / 1000;
    String raw_json = http.getString();

    Serial.print("[Google Auth Handshake Verify] Auth Response = ");
    Serial.println(raw_json);

    DynamicJsonDocument doc(2048);

    deserializeJson(doc, raw_json);

    const char *access_token_src = doc["access_token"];
    int expires_in_src = doc["expires_in"];
    const char *refresh_token_src = doc["refresh_token"];
    const char *scope = doc["scope"];
    const char *token_type = doc["token_type"];
    const char *id_token = doc["id_token"];

    Serial.print("[Google Auth Handshake Verify] Access token = ");
    Serial.println(access_token_src);
    Serial.print("[Google Auth Handshake Verify] Expires in = ");
    Serial.println(expires_in_src);
    Serial.print("[Google Auth Handshake Verify] Refresh token = ");
    Serial.println(refresh_token_src);
    Serial.print("[Google Auth Handshake Verify] Scope = ");
    Serial.println(scope);
    Serial.print("[Google Auth Handshake Verify] Token Type = ");
    Serial.println(token_type);
    Serial.print("[Google Auth Handshake Verify] ID token (use <https://jwt.io> to decode) = ");
    Serial.println(id_token);

    Serial.flush();

    strncpy(access_token, access_token_src, MAX_ACCESS_TOKEN_LEN);
    strncpy(refresh_token, refresh_token_src, MAX_REFRESH_TOKEN_LEN);
    auth_token_expires_in = static_cast<unsigned long>(expires_in_src);

    http.end();

    lcd.clear();
    lcd.home();
    lcd.print("Your account is");
    lcd.setCursor(0, 1);
    lcd.print("properly linked!");

    delay(5000);

    return;
  }
}

unsigned long auth_token_seconds_to_expiration() {
  unsigned long will_expire_at_seconds = auth_token_init + auth_token_expires_in;
  unsigned long now_seconds = millis() / 1000;

  if (now_seconds >= will_expire_at_seconds) {
    return 0;
  } else {
    return will_expire_at_seconds - now_seconds;
  }
}

size_t get_current_rfc3339_timestamp(char *timestamp, size_t timestamp_len) {
  struct tm timeinfo;
  init_time_info(&timeinfo);

  // Using %z is okay here, since we specify a time zone at initialization.
  // Google's API accepts time zones as "+HH:mm", "-HH:mm", "+HHmm", or "-HHmm".
  return strftime(timestamp, timestamp_len, "%FT%T%z" /* aka %Y-%m-%dT%H:%M:%S%z */, &timeinfo);
}

#define MAX_TIME_SERVER_REQUEST_ATTEMPTS 7
#define RFC3339_STRING_BUF_LEN 40

void init_time_info(struct tm *timeinfo) {
restart:
  int time_requests;
  wl_status_t wifi_status;
  for (time_requests = 1; (wifi_status = WiFi.status()) == WL_CONNECTED && time_requests <= MAX_TIME_SERVER_REQUEST_ATTEMPTS; time_requests++) {
    if (getLocalTime(timeinfo)) {
      return;
    }

    Serial.printf("[Setup] Failed to obtain time. Have tried %d/7 times. Will retry...\n", time_requests);
    delay(500 * time_requests);
  }

  if (wifi_status != WL_CONNECTED) {
    lcd.clear();
    lcd.home();
    lcd.print("Disconnected!");
    delay(2000);
    connect_to_wifi();
    lcd.clear();
    lcd.home();
    lcd.print("Back online!");
    goto restart;
  }

  // we have internet and the request failed seven times.

  lcd.clear();
  lcd.home();
  lcd.print("No connection to");
  lcd.setCursor(0, 1);
  lcd.print("Time Server");

  // hang
  while (1) {}
}

void setup() {
  lcd.init();  // initialize the lcd
  lcd.clear();
  lcd.createChar(7, customBackslash);

  Serial.begin(9600);

  Serial.println(F(
    "\n\nInit debug port\n\n"
    "  ___ ___ ___ _______    ___      _             _          \n"
    " | __/ __| _ \\__ /_  )  / __|__ _| |___ _ _  __| |__ _ _ _ \n"
    " | _|\\__ \\  _/|_ \\/ /  | (__/ _` | / -_) ' \\/ _` / _` | '_|\n"
    " |___|___/_| |___/___|  \\___\\__,_|_\\___|_||_\\__,_\\__,_|_|  \n"
    "\nMade by Mateo Rodriguez in 2023\n\n"));

  lcd.backlight();

  Serial.println("[Setup] Connecting to " WIFI_SSID);
  connect_to_wifi();

  Serial.println("[Setup] Connecting to NTP Time Server");
  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER, NTP_SERVER_FALLBACK);
  Serial.println("[Setup] Established connection to NTP Time Server");

  char rfc3339_buffer[RFC3339_STRING_BUF_LEN];
  size_t chars_copied = get_current_rfc3339_timestamp(rfc3339_buffer, sizeof(rfc3339_buffer));

  Serial.print("[Setup] The local time is: ");
  Serial.println(rfc3339_buffer);

#ifndef SPEEDY
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
#endif  //SPEEDY

  char device_code[128];
  char user_code[16];
  char verify_url[41];
  int expires_in;

  Serial.println("[Setup] Starting Auth Flow with Google Servers");
  oauth_login(device_code, user_code, &expires_in, verify_url);

  Serial.println("[Setup] Confirming Auth Flow with Google Servers");

  oauth_poll(device_code, user_code, expires_in, verify_url);

#ifndef SPEEDY
  lcd.clear();
  lcd.home();
  lcd.print("Done with setup");

  delay(4000);
#endif  // SPEEDY
}

#define UPDATE_AUTH_TOKEN_BODY "client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&grant_type=refresh_token&refresh_token="

void update_auth_token() {
restart:
  Serial.println("[POST Google Auth Key Update] Refreshing access token...");

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

    HTTPClient http;

    auto begin_code = http.begin(client, GOOGLE_POLL_SERVER);
    Serial.print("[POST Google Auth Key Update] Begin code = ");
    Serial.println(begin_code);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    auto body = String(UPDATE_AUTH_TOKEN_BODY) + refresh_token;
    // char body[sizeof(UPDATE_AUTH_TOKEN_BODY) + MAX_REFRESH_TOKEN_LEN];
    // strcpy(body, UPDATE_AUTH_TOKEN_BODY);
    // strcpy(body + sizeof(UPDATE_AUTH_TOKEN_BODY) - 1, refresh_token);

    Serial.print("[POST Google Auth Key Update] Will POST with parameters: ");

    Serial.println(body.c_str());

    // Serial.print("[POST Google Auth Key Update] Body len: ");
    // size_t body_len = strlen(body);
    // Serial.println(body_len);

    int responseCode = http.POST(body);
    Serial.print("[POST Google Auth Key Update] HTTP Response code = ");

    if (WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(0, 0);
      lcd.print("Lost signal");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
      delay(2000);

      Serial.print("[POST Google Auth Key Update] Lost signal, restarting polling...");

      connect_to_wifi();

      Serial.print("[POST Google Auth Key Update] Back online, restarting polling...");

      http.end();

      goto restart;
    } else if (responseCode < 0) {
      Serial.printf("NET_ERR\n[HTTPS] GET... failed, error: %s. WILL RETRY\n", http.errorToString(responseCode).c_str());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("Retrying..."));

      http.end();

      goto restart;
    }

    Serial.println(responseCode);

    String raw_json = http.getString();

    Serial.print("[POST Google Auth Key Update] Poll returned: ");
    Serial.println(raw_json);

    if (responseCode != 200) {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(responseCode).c_str());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("See serial port"));

      Serial.println(F("[GET Google Calendar API] This error renders the Calendar API inaccessible. Try updating the auth certificate."));

      http.end();

      // hang
      while (1) {}
    }

    DynamicJsonDocument doc(2048);

    deserializeJson(doc, raw_json);

    const char *access_token_src = doc["access_token"];
    int expires_in_src = doc["expires_in"];

    strcpy(access_token, access_token_src);
    auth_token_expires_in = expires_in_src;

  } else {
    Serial.println("[POST Google Auth Key Update] Disconnected");

    connect_to_wifi();

    goto restart;
  }
}

#define GOOGLE_TOKEN_INFO_ENDPOINT "https://www.googleapis.com/oauth2/v1/tokeninfo?access_token="

#define MOCK_NEEDS_AUTH_TOKEN 0

#if MOCK_NEEDS_AUTH_TOKEN
bool app_needs_new_auth_token() {
  Serial.println("[GET Google Auth Key Status*] Mocking \"true\"");
  return true;
}
#else
bool app_needs_new_auth_token() {
restart:
  Serial.println("[GET Google Auth Key Status] Polling validity...");
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

    HTTPClient http;

    char domain[MAX_ACCESS_TOKEN_LEN + sizeof(GOOGLE_TOKEN_INFO_ENDPOINT)];
    strcpy(domain, GOOGLE_TOKEN_INFO_ENDPOINT);
    strcpy(domain + sizeof(GOOGLE_TOKEN_INFO_ENDPOINT) - 1, access_token);

    auto begin_code = http.begin(client, domain);
    Serial.print("[GET Google Auth Key Status] Begin code = ");
    Serial.println(begin_code);

    http.addHeader("Accept", "application/json");

    int responseCode = http.GET();
    Serial.print("[GET Google Auth Key Status] HTTP Response code = ");

    if (WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(0, 0);
      lcd.print("Lost signal");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
      delay(2000);

      Serial.print("[GET Google Auth Key Status] Lost signal, restarting polling...");

      connect_to_wifi();

      Serial.print("[GET Google Auth Key Status] Back online, restarting polling...");

      http.end();

      goto restart;
    } else if (responseCode < 0) {
      Serial.printf("NET_ERR\n[HTTPS] GET... failed, error: %s. WILL RETRY\n", http.errorToString(responseCode).c_str());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("Retrying..."));

      http.end();

      goto restart;
    }

    Serial.println(responseCode);

    String raw_json = http.getString();

    Serial.print("[GET Google Auth Key Status] Poll returned: ");
    Serial.println(raw_json);

    http.end();

    if (responseCode != 200) {
      Serial.print("[GET Google Auth Key Status] Got an HTTP error status ");
      Serial.println(responseCode);

      Serial.println("[GET Google Auth Key Status] Assuming this error means we need a new access token.");

      return true;
    }

    Serial.println("[GET Google Auth Key Status] We got status code 200, which means the key is still valid");

    return false;
  } else {
    Serial.println("[GET Google Auth Key Status] Disconnected");

    connect_to_wifi();

    goto restart;
  }
}
#endif  // app_needs_new_auth_token()

int counter = 1;

// +1 space for null byte
#define MAX_UI_DISPLAY_FOR_EVENT_NAME 33

#define GOOGLE_CALDENDAR_API "https://www.googleapis.com/calendar/v3/calendars/primary/events?maxResults=1&orderBy=startTime&singleEvents=true&fields=items%2Fsummary%2Citems%2Fstart%2FdateTime%2Citems%2Fend%2FdateTime&key=" CALENDAR_API_KEY "&timeMin="

void get_next_calendar_event(char event_name[MAX_UI_DISPLAY_FOR_EVENT_NAME], char start_date[RFC3339_STRING_BUF_LEN], char end_date[RFC3339_STRING_BUF_LEN]) {
restart:
  lcd.clear();
  lcd.home();
  lcd.print("Refreshing...");

  Serial.print("[GET Google Calendar API] Refreshing calendar... (Auth key is valid for ");
  Serial.print(auth_token_seconds_to_expiration());
  Serial.println(" more seconds)");

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_GOOGLE_AUTH_SERVER);

    HTTPClient http;

    char time_min_buffer[RFC3339_STRING_BUF_LEN];
    size_t chars_read = get_current_rfc3339_timestamp(time_min_buffer, sizeof(time_min_buffer));

    if (chars_read == 0) {
      lcd.home();
      lcd.clear();
      lcd.print("Could not format");
      lcd.setCursor(0, 1);
      lcd.print("RFC3339 date");

      Serial.println("[GET Google Calendar API] Could not format date");

      // hang
      while (1) {}
    }

    char domain[sizeof(GOOGLE_CALDENDAR_API) + sizeof(time_min_buffer)];

    strcpy(domain, GOOGLE_CALDENDAR_API);
    strcpy(domain + sizeof(GOOGLE_CALDENDAR_API) - 1 /* nuke the null byte */, time_min_buffer);

    Serial.print("[GET Google Calendar API] Will make request to ");
    Serial.println(domain);

    auto begin_code = http.begin(client, domain);
    Serial.print("[GET Google Calendar API] Begin code = ");
    Serial.println(begin_code);

    char authorization_buffer[MAX_ACCESS_TOKEN_LEN + 7];
    strcpy(authorization_buffer, "Bearer ");
    strcpy(&authorization_buffer[7] /* nuke the null byte */, access_token);

    Serial.print("[GET Google Calendar API] Using HTTP authorization header: ");
    Serial.println(authorization_buffer);

    http.addHeader("Accept", "application/json");
    http.addHeader("Authorization", authorization_buffer);

    int responseCode = http.GET();
    Serial.print("[GET Google Calendar API] HTTP Response code = ");

    if (WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(0, 0);
      lcd.print("Lost signal");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
      delay(2000);

      Serial.print("[GET Google Calendar API] Lost signal, restarting polling...");

      connect_to_wifi();

      Serial.print("[GET Google Calendar API] Back online, restarting polling...");

      http.end();

      goto restart;
    } else if (responseCode < 0) {
      Serial.printf("NET_ERR\n[HTTPS] GET... failed, error: %s. WILL RETRY\n", http.errorToString(responseCode).c_str());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("Retrying..."));

      http.end();

      goto restart;
    }

    Serial.println(responseCode);

    if (responseCode != 200) {
      if (app_needs_new_auth_token()) {
        http.end();
        update_auth_token();
        goto restart;
      }

      Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(responseCode).c_str());

      Serial.print(http.getString());

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Auth failed #"));
      lcd.print(responseCode);

      lcd.setCursor(0, 1);
      lcd.print(F("See serial port"));

      Serial.println(F("[GET Google Calendar API] This error renders the Calendar API inaccessible. Try updating the auth certificate."));

      http.end();

      // hang
      while (1) {}
    }

    String raw_json = http.getString();

    Serial.print("[GET Google Calendar API] Auth Response (JSON) = ");
    Serial.println(raw_json);




    // die for now... work on this tomorrow!
    // while (1) {}
    DynamicJsonDocument doc(2048);

    deserializeJson(doc, raw_json);

    const char *name = doc["items"][0]["summary"];
    const char *start_time = doc["items"][0]["start"]["dateTime"];
    const char *end_time = doc["items"][0]["end"]["dateTime"];

    Serial.print("[GET Google Calendar API] name = ");
    Serial.println(name);

    Serial.print("[GET Google Calendar API] start_time = ");
    Serial.println(start_time);

    Serial.print("[GET Google Calendar API] end_time = ");
    Serial.println(end_time);

    http.end();

  } else {
    Serial.println("[INIT Google Auth Handshake] Disconnected");

    connect_to_wifi();

    goto restart;
  }
}

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

  // useAccessToken();

  lcd.print("GET calendar");
  lcd.setCursor(0, 1);
  lcd.print(counter++);

  // Serial.println(access_token);
  // Serial.println(refresh_token);
  // Serial.println(auth_token_expires_in);

  char event_name[MAX_UI_DISPLAY_FOR_EVENT_NAME];
  char start_date[RFC3339_STRING_BUF_LEN];
  char end_date[RFC3339_STRING_BUF_LEN];

  get_next_calendar_event(event_name, start_date, end_date);

  delay(10000);
}