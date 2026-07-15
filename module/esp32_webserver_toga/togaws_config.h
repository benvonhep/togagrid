#ifndef TOGAWS_CONFIG_H
#define TOGAWS_CONFIG_H
// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
class Config {
public:

  void loadConfiguration(const char *filename);
  void loadConfiguration(const String &str);
  bool writeConfiguration(File file);
  bool writeConfiguration(String &str);
  bool saveConfiguration(const char *filename);
  void printFile(const char *filename);

  char hostname[64];
  int leds;
};

#endif