#pragma once

#ifdef NATIVE_TEST
// =========================================================================
// MOCKS PARA O AMBIENTE NATIVE (PC)
// =========================================================================
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdint>

static unsigned long _simulated_millis = 0;
inline unsigned long millis()
{
    _simulated_millis += 10;
    return _simulated_millis;
}
inline void noInterrupts() {}
inline void interrupts() {}

class String : public std::string
{
public:
    String() : std::string() {}
    String(const char *s) : std::string(s) {}
    String(const std::string &s) : std::string(s) {}

    String(float f, int decimals)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, f);
        this->assign(buf);
    }

    String(int i) : std::string(std::to_string(i)) {}
    String(unsigned int i) : std::string(std::to_string(i)) {}
    String(long l) : std::string(std::to_string(l)) {}

    // --- ADICIONE ESTA LINHA ---
    void reserve(size_t n) { std::string::reserve(n); }
    // ---------------------------

    String &operator+=(const String &other)
    {
        std::string::operator+=(other);
        return *this;
    }
    String &operator+=(const char *c)
    {
        std::string::operator+=(c);
        return *this;
    }
    bool operator==(const String &other) const { return this->compare(other) == 0; }

    size_t length() const { return std::string::length(); }
    char operator[](size_t i) const { return std::string::operator[](i); }
    const char *c_str() const { return std::string::c_str(); }

    int indexOf(const String &str) const
    {
        size_t pos = this->find(str);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
};

#else
#include <Arduino.h>
#endif