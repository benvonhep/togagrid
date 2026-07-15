#ifndef TOGAWS_GLOBALS_H
#define TOGAWS_GLOBALS_H

#define LED_RED 0
#define LED_BLUE 2

typedef void (*SimplePatternList[])();
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) {
  return N;
}


#endif
