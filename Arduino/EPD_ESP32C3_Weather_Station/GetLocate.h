
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// Grid 구조체 정의
struct Grid {
  int x;
  int y;
};

// dfs_xy_conv 함수 선언
Grid dfs_xy_conv(const char* code, double v1, double v2);

// Location 구조체 정의
struct Location {
  String country;
  String regionName;
  String city;
  int nx;
  int ny;
};

Location getLocation() {
  HTTPClient http;
  Location loc;
  http.begin("http://ip-api.com/json");
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    JSONVar doc = JSON.parse(payload);

    if (JSON.typeof(doc) != "undefined" && strcmp((const char*)doc["status"], "success") == 0) {
      loc.country = (const char*)doc["country"];
      loc.regionName = (const char*)doc["regionName"];
      loc.city = (const char*)doc["city"];
      float lat = (double)doc["lat"];
      float lon = (double)doc["lon"];

      // 좌표를 NX, NY로 변환
      auto grid = dfs_xy_conv("toXY", lat, lon);
      loc.nx = grid.x;
      loc.ny = grid.y;
    } else {
      Serial.println("Failed to parse location data");
    }
  } else {
    Serial.println("Failed to retrieve location data");
  }
  http.end();
  return loc;
}

// dfs_xy_conv 함수 정의
Grid dfs_xy_conv(const char* code, double v1, double v2) {
  const double RE = 6371.00877;
  const double GRID = 5.0;
  const double SLAT1 = 30.0;
  const double SLAT2 = 60.0;
  const double OLON = 126.0;
  const double OLAT = 38.0;
  const double XO = 43;
  const double YO = 136;

  const double DEGRAD = M_PI / 180.0;
  const double RADDEG = 180.0 / M_PI;

  double re = RE / GRID;
  double slat1 = SLAT1 * DEGRAD;
  double slat2 = SLAT2 * DEGRAD;
  double olon = OLON * DEGRAD;
  double olat = OLAT * DEGRAD;

  double sn = tan(M_PI * 0.25 + slat2 * 0.5) / tan(M_PI * 0.25 + slat1 * 0.5);
  sn = log(cos(slat1) / cos(slat2)) / log(sn);
  double sf = tan(M_PI * 0.25 + slat1 * 0.5);
  sf = pow(sf, sn) * cos(slat1) / sn;
  double ro = tan(M_PI * 0.25 + olat * 0.5);
  ro = re * sf / pow(ro, sn);

  Grid rs = {0, 0};
  if (strcmp(code, "toXY") == 0) {
    double ra = tan(M_PI * 0.25 + (v1) * DEGRAD * 0.5);
    ra = re * sf / pow(ra, sn);
    double theta = v2 * DEGRAD - olon;
    if (theta > M_PI) theta -= 2.0 * M_PI;
    if (theta < -M_PI) theta += 2.0 * M_PI;
    theta *= sn;
    rs.x = floor(ra * sin(theta) + XO + 0.5);
    rs.y = floor(ro - ra * cos(theta) + YO + 0.5);
  }
  return rs;
}
