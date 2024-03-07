#ifndef NANOPACK_MESSAGE_HXX
#define NANOPACK_MESSAGE_HXX

#include <cstdint>
#include <vector>

#include "writer.hxx"
#include "nanopack.hxx"

namespace NanoPack {

class Message {
  public:
	virtual ~Message() = default;

	[[nodiscard]] virtual TypeId type_id() const = 0;

	[[nodiscard]] virtual int header_size() const = 0;

	virtual void write_to(Writer &writer) const = 0;

	[[nodiscard]] virtual std::vector<uint8_t> data() const = 0;

	[[nodiscard]] virtual std::vector<uint8_t>
	data_with_length_prefix() const = 0;
};

} // namespace NanoPack

#endif // NANOPACK_MESSAGE_HXX
