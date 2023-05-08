// Часы на вакуумно-люминесцентном индикаторе M202MD25CA
//
// Подключение к CN3: RX, _, +12v, GND
// Перемычку J3 замкнуть
// Ссылки на библиотеки:
// — NixdorfVFD https://github.com/MrTransistorsChannel/NixdorfVFD
// — NTPClient https://github.com/arduino-libraries/NTPClient
// — GotchauTimings https://github.com/gotchau/GotchauTimings
//
// Shorts https://youtu.be/WjhaW8w6p2g
// Gotchau 05.05.23

#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <NixdorfVFD.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <GotchauTimings.h>

const char *deviceHostname = "GotchauClockVFD";

#ifndef STASSID
#define STASSID "your-wifi"
#define STAPSK "password"
#endif

const long utcOffsetInSeconds = 3 * 60 * 60; // Смещение часового пояса в секундах
char daysOfTheWeek[7][23] = {"Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

NixdorfVFD VFD;
SoftwareSerial srl(255, 2, true); // Подключаем программный Serial: RX (не используется), TX, инверсия логики (да)

Timing screenUpdate(1000, Period, Millis); // Создание таймера с периодом в 1 секунду

void setup()
{
  srl.begin(9600);         // Запускаем serial на скорости 9600
  VFD.begin(srl);          // Инициализация дисплея
  VFD.clear();             // Очистка дисплея
  VFD.home();              // Возвращаем курсор в домашнюю позицию
  VFD.setCodePage(CP_866); // Подключаем кириллическую раскладку

  delay(500);

  VFD.setCursor(2, 0);         // Ставим крусор
  VFD.print("Готчасики ВЛИ!"); // Печатаем текст
  VFD.setCursor(7, 1);
  VFD.print("v0.01");

  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(deviceHostname);

  VFD.clear();                     // Очистка дисплея
  VFD.home();                      // Возвращаем курсор в домашнюю позицию
  VFD.print("Подключение к WiFi"); // Печатаем текст

  VFD.setCursor(0, 1);

  WiFi.begin(STASSID, STAPSK);
  for (int i = 0; WiFi.status() != WL_CONNECTED; i++)
  {
    VFD.print("#");
    delay(500);
    if (i > 10)
      break;
  }

  if (WiFi.isConnected())
  {
    VFD.clear();                // Очистка дисплея
    VFD.home();                 // Возвращаем курсор в домашнюю позицию
    VFD.println("Подключено:"); // Печатаем текст

    VFD.print(WiFi.localIP());
    delay(500);

    ArduinoOTA.setHostname(deviceHostname);

    ArduinoOTA.onStart([]()
                       {
                         VFD.clear();             // Очистка дисплея
                         VFD.home();              // Возвращаем курсор в домашнюю позицию
                         VFD.println("Прошивка"); // Печатаем текст
                       });

    ArduinoOTA.onEnd([]()
                     {
                       VFD.clear();                         // Очистка дисплея
                       VFD.home();                          // Возвращаем курсор в домашнюю позицию
                       VFD.println("Прошивка завершена:)"); //
                       VFD.print("Перезагрузка...");        //
                     });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
                            // VFD.clear();                                                // Очистка дисплея
                            VFD.home();                                                 // Возвращаем курсор в домашнюю позицию
                            VFD.println("Выполняется прошивка");                        // Печатаем текст
                            VFD.printf("Прогресс: %u%%\r", (progress / (total / 100))); // Отображение прогресса
                          });

    ArduinoOTA.begin();
    timeClient.begin();
  }
  else
  {
    VFD.clear();                     // Очистка дисплея
    VFD.home();                      // Возвращаем курсор в домашнюю позицию
    VFD.println("Не удалось");       // Печатаем текст
    VFD.print("   подключиться :("); // Печатаем текст

    while (1)
      ;
  }

  screenUpdate.start();

  VFD.clear(); // Очистка дисплея
}

void loop()
{
  ArduinoOTA.handle();

  if (screenUpdate.timeHasCome())
  {
    timeClient.update(); // Обновление времени (каждые 60с синхронизация)

    VFD.home();

    char *s = daysOfTheWeek[timeClient.getDay()];
    VFD.printf("%-20s\n", s);

    VFD.setCursor(6, VFD.cursorY());
    VFD.printf("%02d", timeClient.getHours());
    VFD.print(":");
    VFD.printf("%02d", timeClient.getMinutes());
    VFD.print(":");
    VFD.printf("%02d", timeClient.getSeconds());
  }
}