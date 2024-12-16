#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi 설정
struct WiFiCredentials {
    const char* ssid;
    const char* password;
};

// 여러 개의 와이파이 정보 설정
const WiFiCredentials wifiNetworks[] = {
    {"jongsfold", "nrax5375"},     // 첫 번째 와이파이
    {"8th_floor_home", "ryanajoosweet"}, // 두 번째 와이파이
   // {"another_wifi", "another_pass"} // 세 번째 와이파이
};
const int numNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// NTP 설정
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 9 * 3600, 60000); // 서울 기준 (GMT+9)

// API 요청 데이터
String base_time;
String base_date;
String basedatetime;

// 현재 위치 정보 (서울 성북구 임시 설정)
const int location_x = 61; // 예: nx 값
const int location_y = 127; // 예: ny 값

// API 인증키
const char* api_key = "3BXdtOYTQgwY%2FncPf6kCtZiX8FDuzXs8Pd66HbbAcSpsYrF%2BRz3gmttlkhF3oCOyESziNpJ5kCeBNk%2Bg1%2BC1pA%3D%3D";



// API URL
//단기예보
const char* ultra_srt_ncst_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtNcst"; // 초단기 실황조회
const char* ultra_srt_fcst_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst";  // 초단기 예보조회
const char* vilage_fcst_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getVilageFcst"; // 단기예보조회
const char* fcst_version_url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getFcstVersion"; // 예보버전조회

//중기예보/
/*

*/



void setup() {
  Serial.begin(115200);

  // 와이파이 연결 시도
  bool connected = false;
  Serial.println("Attempting to connect to WiFi...");
  
  for (int i = 0; i < numNetworks && !connected; i++) {
    Serial.printf("\nTrying to connect to %s\n", wifiNetworks[i].ssid);
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    
    // 10초 동안 연결 시도
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.printf("\nConnected to %s successfully!\n", wifiNetworks[i].ssid);
      Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("\nFailed to connect to %s\n", wifiNetworks[i].ssid);
      WiFi.disconnect();
    }
  }
  
  if (!connected) {
    Serial.println("\nFailed to connect to any WiFi network!");
    // 여기서 실패 처리를 할 수 있습니다 (예: ESP32 재시작)
    ESP.restart();
  }

  // 2. NTP 서버에서 시간 가져오기
  timeClient.begin();
  timeClient.setTimeOffset(32400); // 한국 시간 (UTC+9)
  
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
  
  // 현재 시간 출력 추가
  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());

  // base_time과 base_date 계산
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int baseHour = (currentMinute >= 40) ? currentHour : currentHour - 1;
  if (baseHour < 0) baseHour = 23; // 전날로 넘어가는 경우 처리
  base_time = String(baseHour < 10 ? "0" : "") + String(baseHour) + "00";

  // 날짜 계산을 위한 타임스탬프 사용
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  base_date = String(ptm->tm_year + 1900) +
              String(ptm->tm_mon + 1 < 10 ? "0" : "") + String(ptm->tm_mon + 1) +
              String(ptm->tm_mday < 10 ? "0" : "") + String(ptm->tm_mday);

  Serial.println("Base Time: " + base_time);
  Serial.println("Base Date: " + base_date);

  // 중기예보용 basedatetime 계산
  basedatetime = base_date + base_time;
  Serial.println("BaseDateTime for 중기예보: " + basedatetime);

  // 3. 현재 위치 호출 (임시로 서울 성북구 지정)
  Serial.println("Location set to 성북구 (nx: " + String(location_x) + ", ny: " + String(location_y) + ")");

  // 4. 단기예보 API 호출
  callUltraSrtNcstAPI(); // 초단기 실황조회
  callUltraSrtFcstAPI(); // 초단기 예보조회
  callVilageFcstAPI();   // 단기예보조회
  callFcstVersionAPI();  // 예보버전조회
}

void callAPI(const char* api_url, String params) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(api_url) + params;
    
    http.begin(url);
    int httpResponseCode = http.GET();
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  }
}

void callUltraSrtNcstAPI() { // 초단기 실황조회
  Serial.println("초단기 실황조회 호출");
  String params = "?serviceKey=" + String(api_key) +
                  "&numOfRows=10&pageNo=1" +
                  "&dataType=JSON" +
                  "&base_date=" + base_date +
                  "&base_time=" + base_time +
                  "&nx=" + String(location_x) +
                  "&ny=" + String(location_y);
  callAPI(ultra_srt_ncst_url, params);
}

void callUltraSrtFcstAPI() { // 초단기 예보조회
  Serial.println("초단기 예보조회 호출");
  String params = "?serviceKey=" + String(api_key) +
                  "&numOfRows=10&pageNo=1" +
                  "&dataType=JSON" +
                  "&base_date=" + base_date +
                  "&base_time=" + base_time +
                  "&nx=" + String(location_x) +
                  "&ny=" + String(location_y);
  callAPI(ultra_srt_fcst_url, params);
}



void callVilageFcstAPI() {
  Serial.println("단기예보조회 호출");
    int fcstHours[] = {2, 5, 8, 11, 14, 17, 20, 23};
    int currentHour = timeClient.getHours();
    String current_base_date = base_date;
    String current_base_time;
    
    // 현재 시각보다 작은 가장 최근의 발표시각 찾기
    int baseHourIndex = 7;
    for(int i = 7; i >= 0; i--) {
        if(currentHour >= fcstHours[i]) {
            baseHourIndex = i;
            break;
        }
    }

    // 최대 3번 시도
    for(int attempt = 0; attempt < 3; attempt++) {
        // base_time 설정
        current_base_time = String(fcstHours[baseHourIndex] < 10 ? "0" : "") 
                           + String(fcstHours[baseHourIndex]) + "00";

        // 02시보다 이전으로 가야하는 경우 날짜 변경
        if(baseHourIndex < 0) {
            // 전날 23시로 설정
            baseHourIndex = 7;  // 23시 인덱스
            // 전날 날짜 계산
            time_t epochTime = timeClient.getEpochTime() - 86400; // 24시간 전
            struct tm *ptm = gmtime((time_t *)&epochTime);
            current_base_date = String(ptm->tm_year + 1900) +
                              String(ptm->tm_mon + 1 < 10 ? "0" : "") + String(ptm->tm_mon + 1) +
                              String(ptm->tm_mday < 10 ? "0" : "") + String(ptm->tm_mday);
        }

        String params = "?serviceKey=" + String(api_key) +
                       "&numOfRows=10&pageNo=1" +
                       "&dataType=JSON" +
                       "&base_date=" + current_base_date +
                       "&base_time=" + current_base_time +
                       "&nx=" + String(location_x) +
                       "&ny=" + String(location_y);

        // API 호출 및 응답 확인
        if(WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = String(vilage_fcst_url) + params;
            http.begin(url);
            int httpResponseCode = http.GET();

            if(httpResponseCode > 0) {
                String response = http.getString();
                // NO_DATA 체크
                if(response.indexOf("\"resultCode\":\"03\"") == -1) {
                    // 데이터가 있으면 공
                    Serial.println("단기예보 데이터 찾음 - base_date: " + current_base_date + 
                                 ", base_time: " + current_base_time);
                    Serial.println(response);
                    http.end();
                    return;
                }
            }
            http.end();
        }

        // 이전 시간대로 이동
        baseHourIndex--;
        Serial.println("데이터 없음, 이전 시간대 시도 - attempt: " + String(attempt + 1));
    }
    
    Serial.println("단기예보 데이터를 찾을 수 없습니다.");
}

void callFcstVersionAPI() {
  Serial.println("예보버전조회 호출");
  String params = "?serviceKey=" + String(api_key) +
                  "&numOfRows=10" +
                  "&pageNo=1" +
                  "&dataType=JSON" +
                  "&ftype=ODAM" +
                  "&basedatetime=" + basedatetime;
  callAPI(fcst_version_url, params);
}

void processAPIResponse(String response) {
  if (response.indexOf("\"resultCode\":\"00\"") != -1) {
    // 성공적인 응답 처리
    Serial.println("API 호출 성공");
  } else {
    // 에러 응답 처리
    Serial.println("API 호출 실패");
    Serial.println(response);
  }
}

void loop() {
  // 추후 API 호출 및 데이터 처리 반복
}
