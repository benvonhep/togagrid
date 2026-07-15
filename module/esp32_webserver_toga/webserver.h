#ifndef TOGAWS_WEBSERVER_H
#define TOGAWS_WEBSERVER_H

class TogaWebServer {
public:
  void begin();
  static bool exists(String path);
  static String getContentType(String filename);
  static bool handleFileRead(String path);
  static void handleConfig();
  static void handleConfigPost();
  static void handleStore();
  static void handleEffect();
  static void handleColor();
  static void handleSettings();
  static void handleNotFound();
};

#endif
