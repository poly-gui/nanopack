// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND.

#ifndef __Person_NP_H__
#define __Person_NP_H__

#include <nanopack/nanobuf.hxx>
#include <string>

struct Person {
private:
  static const int FIELD_COUNT = 3;

public:
  static const int32_t TYPE_ID = 1;

  std::string first_name;
  std::string last_name;
  int32_t age;
  Person();

  explicit Person(std::vector<uint8_t> &data);

  NanoBuf data();
};

#endif
