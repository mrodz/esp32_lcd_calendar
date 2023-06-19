/*
  ___ ___ ___ _______    ___      _             _          
 | __/ __| _ \__ /_  )  / __|__ _| |___ _ _  __| |__ _ _ _ 
 | _|\__ \  _/|_ \/ /  | (__/ _` | / -_) ' \/ _` / _` | '_|
 |___|___/_| |___/___|  \___\__,_|_\___|_||_\__,_\__,_|_|

                        Config

Fill out these forms according to your application's needs. Once done,
remove the ".SAMPLE" from the file's path and recompile.

See https://developers.google.com/identity/protocols/oauth2/limited-input-device
for an official breakdown of the authentication + authorization protocol.

Enjoy!
*/

#ifndef __ESP32_CALENDAR_CREDENTIALS_H
#define __ESP32_CALENDAR_CREDENTIALS_H

// #define SPEEDY                            // Un-comment this line to get faster test builds (skip some trivial dialogue)

/* START Wifi Config */
#define WIFI_SSID "<YOUR WIFI SSID>"         // your network name
#define WIFI_PASSWORD "<YOUR WIFI PASSWORD>" // your wifi password
/* END Wifi Config */

/* START Time Config */
#define NTP_SERVER "pool.ntp.org"            // this is the default time server, used to sync the ESP32's clock
#define NTP_SERVER_FALLBACK "time.nist.gov"  // if the above server is down, will try this server instead
#define GMT_OFFSET_SECONDS (-28800)          // https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
#define OBSERVES_DAYLIGHT_SAVINGS true       // whether your region observes daylight savings
#if OBSERVES_DAYLIGHT_SAVINGS
#define DAYLIGHT_OFFSET_SECONDS 3600         // +/- one hour
#else
#define DAYLIGHT_OFFSET_SECONDS 0            // no change
#endif // OBSERVES_DAYLIGHT_SAVINGS
/* END Time Config */

/* START OAuth2 App Config */
#define CLIENT_ID "<YOUR CLIENT ID>"         // See https://console.cloud.google.com/apis/library for these secrets.
#define CLIENT_SECRET "<YOUR CLIENT SECRET>" // Note: the Oauth2 App **MUST** be of type "Limited Input Device". 
/* END Oauth2 App Config */

/* START Google Api Config */
#define CALENDAR_API_KEY "<YOUR API KEY>"    // See https://console.cloud.google.com/apis/credentials
/* END Google Api Config */

#endif // __ESP32_CALENDAR_CREDENTIALS_H