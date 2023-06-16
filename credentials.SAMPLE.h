#ifndef __ESP32_CREDENTIALS_H
#define __ESP32_CREDENTIALS_H


#define WIFI_SSID "<WIFI SSID HERE>"
#define WIFI_PASSWORD "<WIFI PASSWORD HERE>"

/*
APP TOKENS:
See https://console.cloud.google.com/apis/credentials/oauthclient
*/

// looks like: <abcdefghijklmnop0123456>.apps.googleusercontent.com"
#define CLIENT_ID "<CLIENT ID HERE>"

// looks like: ABCDEF-abc123 ...
#define CLIENT_SECRET "<CLIENT SECRET HERE>"

#define OAUTH_INIT_ENDPOINT_BODY "client_id="CLIENT_ID"&scope=email%20profile%20https://www.googleapis.com/auth/calendar.readonly"

#endif