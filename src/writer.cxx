#include <nanopack/writer.hxx>

NanoPack::Writer::Writer(std::vector<uint8_t> *buffer) : Writer(buffer, 0) {}

NanoPack::Writer::Writer(std::vector<uint8_t> *buffer, const int offset)
	: offset(offset), buffer(buffer) {}

void NanoPack::Writer::write_type_id(const int32_t type_id) {
	write_int32(type_id, 0);
}

void NanoPack::Writer::write_field_size(const int field_number,
										const int32_t size) {
	write_int32(size, sizeof(int32_t) * (field_number + 1));
}

void NanoPack::Writer::write_int32(const int32_t num, const int offset) {
	const int p = offset + this->offset;
	(*buffer)[p] = num & 0x000000FF;
	(*buffer)[p + 1] = (num & 0x0000FF00) >> 8;
	(*buffer)[p + 2] = (num & 0x00FF0000) >> 16;
	(*buffer)[p + 3] = (num & 0xFF000000) >> 24;
}

void NanoPack::Writer::append_int8(const int8_t num) {
	buffer->emplace_back(num);
}

void NanoPack::Writer::append_int32(const int32_t num) {
	buffer->emplace_back(num & 0x000000FF);
	buffer->emplace_back((num & 0x0000FF00) >> 8);
	buffer->emplace_back((num & 0x00FF0000) >> 16);
	buffer->emplace_back((num & 0xFF000000) >> 24);
}

void NanoPack::Writer::append_int64(const int64_t num) {
	buffer->emplace_back(num & 0x00000000000000FF);
	buffer->emplace_back((num & 0x000000000000FF00) >> 8);
	buffer->emplace_back((num & 0x0000000000FF0000) >> 16);
	buffer->emplace_back((num & 0x00000000FF000000) >> 24);
	buffer->emplace_back((num & 0x000000FF00000000) >> 32);
	buffer->emplace_back((num & 0x0000FF0000000000) >> 40);
	buffer->emplace_back((num & 0x00FF000000000000) >> 48);
	buffer->emplace_back((num & 0xFF00000000000000) >> 56);
}

void NanoPack::Writer::append_string(const std::string &str) {
	buffer->insert(buffer->end(), str.begin(), str.end());
}

void NanoPack::Writer::append_string(const std::string_view &string_view) {
	buffer->insert(buffer->end(), string_view.begin(), string_view.end());
}

void NanoPack::Writer::append_bytes(const std::vector<uint8_t> &bytes) {
	buffer->insert(buffer->end(), bytes.begin(), bytes.end());
}

void NanoPack::Writer::append_bool(const bool b) {
	buffer->emplace_back(b ? 1 : 0);
}

void NanoPack::Writer::append_double(double d) {
	auto *bytes = reinterpret_cast<uint8_t *>(&d);
	buffer->insert(buffer->end(), bytes, bytes + sizeof(double));
}
