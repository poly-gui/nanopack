// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND.

#ifndef PERSON_NP_HXX
#define PERSON_NP_HXX

#include <optional>
#include <string>
#include <vector>

struct Person {
private:
  static const int FIELD_COUNT = 5;

public:
  static const int32_t TYPE_ID = 1;

  std::string first_name;
  std::optional<std::string> middle_name;
  std::string last_name;
  int32_t age;
  std::shared_ptr<Person> other_friend;

  Person();

  Person(std::vector<uint8_t>::const_iterator begin, int &bytes_read);

  std::vector<uint8_t> data();
};

#endif
