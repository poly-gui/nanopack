#ifndef NANOPACK_WRITER_HXX
#define NANOPACK_WRITER_HXX

#include <vector>
#include <string>

namespace NanoPack {

class Writer {
  private:
	std::vector<uint8_t> *buffer;

  public:
	Writer(std::vector<uint8_t> *buffer);

	void write_type_id(int32_t type_id);

	void write_field_size(int field_number, int32_t size);

	void append_int8(int8_t num);

	void append_int32(int32_t num);

	void write_int32(int32_t num, int offset);

	void append_string(const std::string &str);

	void append_bytes(const std::vector<uint8_t> &bytes);

	void append_bool(bool b);

	void append_double(double d);
};

} // namespace NanoPack

#endif // NANOPACK_WRITER_HXX
