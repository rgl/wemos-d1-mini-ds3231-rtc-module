#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>

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
}

void loop() {
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

  delay(5000);
}
