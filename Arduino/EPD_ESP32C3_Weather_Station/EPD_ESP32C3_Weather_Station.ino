#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "Region_DB.h" // Region_DB.h 파일 포함

// 함수 선언
void getShortTermWeather();
void GetMidLandForecast(String city);
void GetMidForecast(String city);
void GetMidTemperature(String city);
void GetMidSeaForecast(String region);
String formatPayload(String payload);

// Wi-Fi 설정
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

WiFiCredentials wifiList[] = {
  {"jongsfold", "nrax5375"},
  {"1F", "!a1b2c3d4e5"},
  {"8th_floor_home", "ryanajoosweet"},
  {"SK_WiFiGIGA304C", "1808020722"}
};

const char* apiKey = "3BXdtOYTQgwY%2FncPf6kCtZiX8FDuzXs8Pd66HbbAcSpsYrF%2BRz3gmttlkhF3oCOyESziNpJ5kCeBNk%2Bg1%2BC1pA%3D%3D";
const long utcOffsetInSeconds = 3600 * 9; // 한국 표준시 (UTC+9)

// 타임서버 설정
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// 업데이트 간격 (예: 10분)
unsigned long updateInterval = 600000;
unsigned long previousMillis = 0;

// 서울 성북구의 좌표 (NX, NY)
const int NX = 61; // X 좌표
const int NY = 127; // Y 좌표

void connectToWiFi() {
  for (WiFiCredentials wifi : wifiList) {
    WiFi.begin(wifi.ssid, wifi.password);
    delay(3000); // 3초 대기

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi: " + String(wifi.ssid));
      return;
    }
  }
  
  Serial.println("Failed to connect to any WiFi network");
}

void setup() {
  Serial.begin(115200);

  connectToWiFi();

  // 타임서버 연결
  timeClient.begin();
  timeClient.update();

  // 초기 날씨 정보 업데이트
  getWeatherUpdate();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= updateInterval) {
    previousMillis = currentMillis;

    // 타임서버 업데이트
    timeClient.update();

    // 현재 시간 출력
    Serial.print("Current time: ");
    Serial.println(timeClient.getFormattedTime());

    // 날씨 정보 업데이트
    getWeatherUpdate();
  }
}

// 날씨 정보를 업데이트하는 함수
void getWeatherUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    getShortTermWeather();
    Serial.println();
    GetMidLandForecast("Seoul"); // 'Seoul'을 사용하여 중기 육상예보 가져오기
    Serial.println();
    GetMidForecast("Seoul"); // 'Seoul'을 사용하여 중기 전망조회
    Serial.println();
    GetMidTemperature("Seoul"); // 'Seoul'을 사용하여 중기 기온조회
    Serial.println();
    GetMidSeaForecast("동해북부"); // '동해북부'를 사용하여 중기 해상예보 가져오기
    Serial.println();
    Serial.println();
  }
}

// 단기예보를 가져오는 함수
void getShortTermWeather() {
  // 현재 날짜와 시간을 가져와 YYYYMMDD 및 HHMM 형식으로 반환
  time_t now = timeClient.getEpochTime(); // 현재 epoch 시간 가져오기
  struct tm* timeinfo = localtime(&now); // epoch 시간을 로컬 시간으로 변환

  // 날짜를 YYYYMMDD 형식으로 포맷팅
  char dateBuffer[11]; // 날짜를 저장할 버퍼
  strftime(dateBuffer, sizeof(dateBuffer), "%Y%m%d", timeinfo); // 날짜를 YYYYMMDD 형식으로 포맷팅
  String currentDate = String(dateBuffer); // 문자열로 변환

  // 시간을 HHMM 형식으로 포맷팅 (매시각 30분 이후에 데이터가 생성되므로 40분 이전에는 이전 시간을 사용)
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;
  if (minute < 40) {
    hour -= 1;
  }
  if (hour < 0) {
    hour += 24;
    timeinfo->tm_mday -= 1;
    strftime(dateBuffer, sizeof(dateBuffer), "%Y%m%d", timeinfo); // 날짜를 YYYYMMDD 형식으로 포맷팅
    currentDate = String(dateBuffer); // 문자열로 변환
  }
  char timeBuffer[5]; // 시간을 저장할 버퍼
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d00", hour); // 시간을 HH00 형식으로 포맷팅
  String currentTime = String(timeBuffer); // 문자열로 변환

  HTTPClient http;
  String weatherURL = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst";
  weatherURL += "?serviceKey=" + String(apiKey);
  weatherURL += "&numOfRows=60&pageNo=1&base_date=" + currentDate;
  weatherURL += "&base_time=" + currentTime + "&nx=" + String(NX) + "&ny=" + String(NY);
  weatherURL += "&dataType=JSON"; // JSON 형식으로 요청

  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    JSONVar response = JSON.parse(payload);

    if (JSON.typeof(response) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }

    JSONVar items = response["response"]["body"]["items"]["item"];
    std::map<String, std::map<String, String>> forecastData;

    for (int i = 0; i < items.length(); i++) {
      String fcstTime = (const char*)items[i]["fcstTime"];
      String category = (const char*)items[i]["category"];
      String fcstValue = (const char*)items[i]["fcstValue"];

      forecastData[fcstTime][category] = fcstValue;
    }

    Serial.println("시간 - 기온 - 하늘상태 - 습도 - 강수량 - 강수형태 - 낙뢰 - 풍향 - 풍속");
    for (const auto& timeEntry : forecastData) {
      String fcstTime = timeEntry.first;
      const auto& data = timeEntry.second;

      String T1H = data.count("T1H") ? data.at("T1H") + "℃" : "-";
      String SKY = data.count("SKY") ? data.at("SKY") : "-";
      String REH = data.count("REH") ? data.at("REH") + "%" : "-";
      String RN1 = data.count("RN1") ? data.at("RN1") : "-";
      String PTY = data.count("PTY") ? data.at("PTY") : "-";
      String LGT = data.count("LGT") ? data.at("LGT") : "-";
      String VEC = data.count("VEC") ? data.at("VEC") + "°" : "-";
      String WSD = data.count("WSD") ? data.at("WSD") + "m/s" : "-";

      Serial.print(fcstTime);
      Serial.print(" - ");
      Serial.print(T1H);
      Serial.print(" - ");
      Serial.print(SKY);
      Serial.print(" - ");
      Serial.print(REH);
      Serial.print(" - ");
      Serial.print(RN1);
      Serial.print(" - ");
      Serial.print(PTY);
      Serial.print(" - ");
      Serial.print(LGT);
      Serial.print(" - ");
      Serial.print(VEC);
      Serial.print(" - ");
      Serial.println(WSD);
    }
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 발표 시각을 YYYYMMDDHHMM 형식으로 반환하는 함수
String getForecastTime() {
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  struct tm* timeinfo = localtime(&now);
  char timeBuffer[15];

  // 현재 시간이 0600 이후이고 1800 이전이면 0600 발표 시각 사용
  // 현재 시간이 1800 이후이면 1800 발표 시각 사용
  if (timeinfo->tm_hour < 6) {
    // 이전 날 18:00 예보 시각 사용
    timeinfo->tm_hour = 18;
    timeinfo->tm_mday -= 1;
    strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d1800", timeinfo);
  } else if (timeinfo->tm_hour < 18) {
    // 같은 날 06:00 예보 시각 사용
    strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d0600", timeinfo);
  } else {
    // 같은 날 18:00 예보 시각 사용
    strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d1800", timeinfo);
  }
  return String(timeBuffer);
}

// 중기전망조회 함수
void GetMidForecast(String city) {
  String midForecastTime = getForecastTime();

  HTTPClient http;
  String weatherURL = "http://apis.data.go.kr/1360000/MidFcstInfoService/getMidFcst";
  weatherURL += "?serviceKey=" + String(apiKey);
  weatherURL += "&numOfRows=10&pageNo=1&stnId=109"; // 서울/인천/경기도 지역의 경우 109 사용
  weatherURL += "&tmFc=" + midForecastTime;
  weatherURL += "&dataType=JSON"; // JSON 형식으로 요청

  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Mid-term Forecast Data: ");
    
    // 특수문자 및 따옴표 제거 및 줄바꿈 출력
    String formattedPayload = formatPayload(payload);
    Serial.println(formattedPayload);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 중기 육상예보조회 함수
void GetMidLandForecast(String city) {
  String midForecastTime = getForecastTime();

  HTTPClient http;
  String weatherURL = "http://apis.data.go.kr/1360000/MidFcstInfoService/getMidLandFcst";
  weatherURL += "?serviceKey=" + String(apiKey);
  weatherURL += "&numOfRows=10&pageNo=1&regId=" + getRegionId(city);
  weatherURL += "&tmFc=" + midForecastTime;
  weatherURL += "&dataType=JSON"; // JSON 형식으로 요청

  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Mid-term Land Forecast Data: ");
    
    // 특수문자 및 따옴표 제거 및 줄바꿈 출력
    String formattedPayload = formatPayload(payload);
    Serial.println(formattedPayload);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 중기 기온조회 함수
void GetMidTemperature(String city) {
  String midForecastTime = getForecastTime();

  HTTPClient http;
  String weatherURL = "http://apis.data.go.kr/1360000/MidFcstInfoService/getMidTa";
  weatherURL += "?serviceKey=" + String(apiKey);
  weatherURL += "&numOfRows=10&pageNo=1&regId=" + getTemperatureRegionId(city);
  weatherURL += "&tmFc=" + midForecastTime;
  weatherURL += "&dataType=JSON"; // JSON 형식으로 요청

  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Mid-term Temperature Data: ");
    
    // 특수문자 및 따옴표 제거 및 줄바꿈 출력
    String formattedPayload = formatPayload(payload);
    Serial.println(formattedPayload);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 중기 해상예보조회 함수
void GetMidSeaForecast(String region) {
  String midForecastTime = getForecastTime();

  HTTPClient http;
  String weatherURL = "http://apis.data.go.kr/1360000/MidFcstInfoService/getMidSeaFcst";
  weatherURL += "?serviceKey=" + String(apiKey);
  weatherURL += "&numOfRows=10&pageNo=1&regId=" + getSeaRegionId(region);
  weatherURL += "&tmFc=" + midForecastTime;
  weatherURL += "&dataType=JSON"; // JSON 형식으로 요청

  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Mid-term Sea Forecast Data: ");
    
    // 특수문자 및 따옴표 제거 및 줄바꿈 출력
    String formattedPayload = formatPayload(payload);
    Serial.println(formattedPayload);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

// 날씨 API의 응답 데이터를 포맷하는 함수
String formatPayload(String payload) {
  payload.replace("{", ""); // '{' 제거
  payload.replace("}", ""); // '}' 제거
  payload.replace("[", ""); // '[' 제거
  payload.replace("]", ""); // ']' 제거
  payload.replace("(", ""); // '(' 제거
  payload.replace(")", ""); // ')' 제거
  payload.replace("\"", ""); // '"' 제거
  payload.replace(",", ",\n"); // ',' 뒤에 줄바꿈 추가
  return payload;
}
