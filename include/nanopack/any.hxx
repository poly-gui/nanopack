#ifndef NANOPACK_ANY_HXX
#define NANOPACK_ANY_HXX

#include "message.hxx"
#include "reader.hxx"

#include <vector>

namespace NanoPack {

class Any {
  private:
	size_t _size;
	std::vector<uint8_t> _data;

  public:
	const size_t &size = _size;
	const std::vector<uint8_t> &data = _data;

	Any(int8_t i);
	Any(int32_t i);
	Any(int64_t i);

	Any(const std::string &string);

	Any(const Message &message);

	Any(std::vector<uint8_t> data);

	[[nodiscard]] Reader as_reader() const;
};

} // namespace NanoPack

#endif // NANOPACK_ANY_HXX
