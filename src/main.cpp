#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

static uint8_t i2c_read_register(uint8_t address, uint8_t register_number) {
  Wire.beginTransmission(address);
  Wire.write(register_number);
  Wire.endTransmission();
  Wire.requestFrom(address, (uint8_t)1);
  return Wire.read();
}

// DS3231 Status Register (0Fh) flags.
// see page 14 in https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
#define DS3231_STATUS_OSF     0x80 // Bit 7: Oscillator Stop Flag (OSF)
#define DS3231_STATUS_EN32KHZ 0x08 // Bit 3: Enable 32kHz Output (EN32kHz)
#define DS3231_STATUS_BSY     0x04 // Bit 2: Busy (BSY)
#define DS3231_STATUS_A2F     0x02 // Bit 1: Alarm 2 Flag (A2F)
#define DS3231_STATUS_A1F     0x01 // Bit 0: Alarm 1 Flag (A1F)

static uint8_t ds3231_get_status() {
  return i2c_read_register(DS3231_ADDRESS, DS3231_STATUSREG);
}

struct StatusFlagText {
  uint8_t flag;
  const char *text;
};

static const StatusFlagText statuses[] = {
  {DS3231_STATUS_OSF, "OSF"},
  {DS3231_STATUS_EN32KHZ, "EN32KHZ"},
  {DS3231_STATUS_BSY, "BSY"},
  {DS3231_STATUS_A2F, "A2F"},
  {DS3231_STATUS_A1F, "A1F"},
};

const uint8_t statuses_length = sizeof(statuses)/sizeof(statuses[0]);

static String ds3231_status_string(uint8_t status) {
  String s;
  for (uint8_t n = 0; n < statuses_length; ++n) {
    if (status & (statuses[n].flag)) {
      if (!s.isEmpty()) {
        s += ",";
      }
      s += statuses[n].text;
    }
  }
  return s;
}

const char* daysOfTheWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static RTC_DS3231 rtc;

const char* wifi_ssid = "TODO";
const char* wifi_pass = "TODO";

static ESP8266WebServer server(80);

bool streamWebUiFile(String path) {
  if (server.method() != HTTP_GET && server.method() != HTTP_HEAD) {
    return false;
  }

  if (path == "/favicon.ico") {
    return false;
  }

  auto is_root_path = path == "/";
  if (is_root_path) {
    path = "/index.html";
  }

  auto path_exists = LittleFS.exists(path);
  auto path_gz = path + ".gz";
  auto path_gz_exists = LittleFS.exists(path_gz);
  auto contentType = mime::getContentType(path);

  // whether to cache all non-index.html resources forever.
  // NB all non-index.html files have a cache busting hash in their
  //    name, e.g., bundle.2ca75.js, and can be cached forever.
  auto cache_forever = false;

  // if the requested file does not exist, its probably because it is a
  // client-side route, and as such, we serve the web-ui.
  if (!path_exists && !path_gz_exists) {
    path_gz_exists = true;
    path_gz = "/index.html.gz";
    contentType = "text/html";
  } else {
    cache_forever = !is_root_path;
  }

  //Serial.printf("Serving path=%s (%d) path_gz=%s (%d)\n", path.c_str(), path_exists, path_gz.c_str(), path_gz_exists);

  // if a <path>.gz file exists, use it.
  // NB almost all of the web-ui files are gzipped (when their compressed
  //    size is smaller than the original).
  File file = LittleFS.open(path_gz_exists ? path_gz : path, "r");
  if (!file) {
    return false;
  }
  if (!file.isFile()) {
      file.close();
      return false;
  }
  if (cache_forever) {
    // NB we are not really caching forever, but for the max-recommended
    //    value of 31536000 (1 year).
    server.sendHeader("Cache-Control", "max-age=31536000");
  }
  server.streamFile(file, contentType, server.method());
  file.close();
  return true;
}

void handleNotFound() {
  String path = ESP8266WebServer::urlDecode(server.uri());
  if (streamWebUiFile(path)) {
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  if (!rtc.begin()) {
    Serial.println("Failed to initialize the RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power; setting its time to " __DATE__ " " __TIME__);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // set the system time (the one returned by time()) from the RTC time.
  // NB behind the scenes the esp8266 runtime sets a timer to increase the
  //    system time every second.
  // NB the system time starts at 0 when the system boots.
  DateTime rtc_time = rtc.now();
  struct timeval tv = {.tv_sec = (time_t)rtc_time.unixtime()};
  settimeofday(&tv, NULL);

  // setup the file system.
  // see https://docs.platformio.org/en/latest/platforms/espressif8266.html#using-filesystem
  LittleFS.begin();

  // // list the file system contents.
  // auto root_dir = LittleFS.openDir("/");
  // while (root_dir.next()) {
  //   Serial.printf("FS %s %s\n", root_dir.isDirectory() ? "dir" : "file", root_dir.fileName().c_str());
  // }

  // connect to the wifi.
  Serial.printf("Connecting to the %s wifi network as %s...", wifi_ssid, WiFi.macAddress().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf(
    "Connected to the %s wifi network as %s and got the %s IP address\n",
    wifi_ssid,
    WiFi.macAddress().c_str(),
    WiFi.localIP().toString().c_str());

  // configure the web server.
  server.on("/state.json", []() {
    time_t system_time = time(NULL);
    DateTime rtc_time = rtc.now();
    uint8_t rtc_status = ds3231_get_status();
    float rtc_temperature = rtc.getTemperature();

    DynamicJsonDocument doc(JSON_OBJECT_SIZE(5));
    doc["rtcStatus"] = rtc_status;
    doc["rtcTime"] = rtc_time.unixtime();
    doc["rtcTemperature"] = rtc_temperature;
    doc["systemTime"] = (int)system_time;
    doc["systemFreeHeap"] = ESP.getFreeHeap();

    server.send(200, "text/json", doc.as<String>());
  });
  server.on("/time.json", []() {
    if (server.method() == HTTP_GET) {
      time_t system_time = time(NULL);
      DynamicJsonDocument doc(JSON_OBJECT_SIZE(1));
      doc["time"] = (int)system_time;
      server.send(200, "text/json", doc.as<String>());
    }
  });
  server.onNotFound(handleNotFound);

  // start the web server.
  server.begin();

  Serial.println("Booted!");
}

static unsigned long next_rtc_print_millis = 0;

void loop() {
  unsigned long m = millis();

  if (m > next_rtc_print_millis) {
    next_rtc_print_millis = m + 30*1000;

    DateTime rtc_time = rtc.now();
    time_t system_time = time(NULL);
    uint8_t rtc_status = ds3231_get_status();
    String rtc_status_flags = ds3231_status_string(rtc_status);

    Serial.printf(
      "%s %d-%02d-%02dT%02d:%02d:%02dZ @%d (rtc) @%d (system) %.2f°C 0x%02x (%s)\n",
      daysOfTheWeek[rtc_time.dayOfTheWeek()],
      rtc_time.year(),
      rtc_time.month(),
      rtc_time.day(),
      rtc_time.hour(),
      rtc_time.minute(),
      rtc_time.second(),
      rtc_time.unixtime(),
      (int)system_time,
      rtc.getTemperature(),
      rtc_status,
      ds3231_status_string(rtc_status).c_str());
  }

  server.handleClient();
}
