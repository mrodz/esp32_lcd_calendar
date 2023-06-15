#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

const char* ssid = "NETGEAR91";
const char* password = "redunicorn340";

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

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

int show = 0;

const __FlashStringHelper* wifi_status_to_string(wl_status_t status) {
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

void setup() {
  lcd.init();  // initialize the lcd
  lcd.clear();
  lcd.createChar(7, customBackslash);

  Serial.begin(9600);
  Serial.println("Init debug port");

  lcd.backlight();
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
    lcd.print(wifi_status_to_string(wifi_status));

    delay(200);
  } while (wifi_status != WL_CONNECTED);

  delay(2000);

  lcd.clear();
  lcd.noDisplay();

  delay(200);

  lcd.setCursor(0, 0);
  lcd.print("Online with IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  lcd.display();

  delay(4000);
}

int counter = 1;

void loop() {
  lcd.clear();
  lcd.home();

  wl_status_t wifi_status = WiFi.status();

  if (wifi_status != WL_CONNECTED) {
    lcd.print("Lost WiFi Signal");

    int attempts = 0;

    do {
      lcd.setCursor(0, 1);
      lcd.print(attempt_chars[attempts++]);
      attempts %= sizeof(attempt_chars);

      lcd.setCursor(2, 1);
      lcd.print("              ");
      lcd.setCursor(2, 1);
      wifi_status = WiFi.status();
      lcd.print(wifi_status_to_string(wifi_status));

      delay(200);
    } while (wifi_status != WL_CONNECTED);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Back online!");
    delay(2000);
    lcd.clear();
  }

  lcd.print("Lorem Ipsum");
  lcd.setCursor(0, 1);
  lcd.print(counter++);

  delay(5000);
}