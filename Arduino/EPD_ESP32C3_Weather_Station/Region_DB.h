#ifndef REGION_DB_H
#define REGION_DB_H

#include <Arduino.h>
#include <map>

// 도시 이름에 따른 중기예보 구역 코드를 반환하는 함수
String getRegionId(String city) {
  static std::map<String, String> regionMap = {
    {"Seoul", "11B00000"},
    {"Incheon", "11B00000"},
    {"Gyeonggi-do", "11B00000"},
    {"Gangwon-do", "11D20000"},
    {"Daejeon", "11C20000"},
    {"Sejong", "11C20000"},
    {"Chungcheongnam-do", "11C20000"},
    {"Chungcheongbuk-do", "11C10000"},
    {"Gwangju", "11F20000"},
    {"Jeollanam-do", "11F20000"},
    {"Jeollabuk-do", "11F10000"},
    {"Daegu", "11H10000"},
    {"Gyeongsangbuk-do", "11H10000"},
    {"Busan", "11H20000"},
    {"Ulsan", "11H20000"},
    {"Gyeongsangnam-do", "11H20000"},
    {"Jeju-do", "11G00000"},
    {"전국", "108"}
  };

  auto it = regionMap.find(city);
  if (it != regionMap.end()) {
    return it->second;
  } else {
    return "108"; // Default: 전국
  }
}

// 도시 이름에 따른 중기기온예보 구역 코드를 반환하는 함수
String getTemperatureRegionId(String city) {
  static std::map<String, String> tempRegionMap = {
    {"Seoul", "11B10101"},
    {"Incheon", "11B20201"},
    {"Gyeonggi-do", "11B20601"},
    {"Gangwon-do", "11D20501"},
    {"Daejeon", "11C20401"},
    {"Sejong", "11C20404"},
    {"Chungcheongnam-do", "11C20101"},
    {"Chungcheongbuk-do", "11C10301"},
    {"Gwangju", "11F20501"},
    {"Jeollanam-do", "21F20801"},
    {"Jeollabuk-do", "11F10201"},
    {"Daegu", "11H10701"},
    {"Gyeongsangbuk-do", "11H10501"},
    {"Busan", "11H20201"},
    {"Ulsan", "11H20101"},
    {"Gyeongsangnam-do", "11H20301"},
    {"Jeju-do", "11G00201"}
  };

  auto it = tempRegionMap.find(city);
  if (it != tempRegionMap.end()) {
    return it->second;
  } else {
    return "108"; // Default: 전국
  }
}

// 해상 예보 구역에 따른 코드 반환 함수
String getSeaRegionId(String region) {
  static std::map<String, String> seaRegionMap = {
    {"서해중부", "12A20000"},
    {"서해남부", "12A30000"},
    {"남해서부", "12B10000"},
    {"남해동부", "12B20000"},
    {"동해남부", "12C10000"},
    {"동해중부", "12C20000"},
    {"동해북부", "12C30000"},
    {"서해북부", "12A10000"},
    {"제주도", "12B10500"},
    {"대화퇴", "12D00000"},
    {"동중국해", "12E00000"},
    {"규슈", "12F00000"},
    {"연해주", "12G00000"}
  };

  auto it = seaRegionMap.find(region);
  if (it != seaRegionMap.end()) {
    return it->second;
  } else {
    return "12A20000"; // Default: 서해중부
  }
}

#endif
