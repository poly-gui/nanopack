#include "cxx_generator.hxx"
#include "../../data_type/np_array.hxx"
#include "../../data_type/np_bool.hxx"
#include "../../data_type/np_double.hxx"
#include "../../data_type/np_int32.hxx"
#include "../../data_type/np_int8.hxx"
#include "../../data_type/np_map.hxx"
#include "../../data_type/np_optional.hxx"
#include "../../data_type/np_string.hxx"
#include "../../string_util/case_conv.hxx"
#include "cxx_array_generator.hxx"
#include "cxx_bool_generator.hxx"
#include "cxx_double_generator.hxx"
#include "cxx_int32_generator.hxx"
#include "cxx_int8_generator.hxx"
#include "cxx_map_generator.hxx"
#include "cxx_message_generator.hxx"
#include "cxx_optional_generator.hxx"
#include "cxx_string_generator.hxx"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>

const std::filesystem::path HEADER_FILE_EXT(".np.hxx");
const std::filesystem::path CODE_FILE_EXT(".np.cxx");

CxxGenerator::CxxGenerator()
	: user_defined_message_type_generator(
		  std::make_shared<CxxMessageGenerator>()),
	  data_type_generator_registry() {
	data_type_generator_registry =
		std::make_shared<DataTypeCodeGeneratorRegistry>();

	data_type_generator_registry->add_generator_for_type(
		NanoPack::Bool::IDENTIFIER, std::make_shared<CxxBoolGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int8::IDENTIFIER, std::make_shared<CxxInt8Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int32::IDENTIFIER, std::make_shared<CxxInt32Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Double::IDENTIFIER, std::make_shared<CxxDoubleGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::String::IDENTIFIER, std::make_shared<CxxStringGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Array::IDENTIFIER,
		std::make_shared<CxxArrayGenerator>(data_type_generator_registry));
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Map::IDENTIFIER,
		std::make_shared<CxxMapGenerator>(data_type_generator_registry));
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Optional::IDENTIFIER,
		std::make_shared<CxxOptionalGenerator>(data_type_generator_registry));
}

void CxxGenerator::generate_for_schema(const MessageSchema &schema) {
	const std::string header_file_name = generate_header_file(schema);
	generate_code_file(schema, header_file_name);
}

std::shared_ptr<DataTypeCodeGenerator>
CxxGenerator::find_generator_for_field(const MessageField &field) {
	std::shared_ptr<DataTypeCodeGenerator> generator =
		data_type_generator_registry->find_generator_for_type(field.type.get());
	if (generator == nullptr) {
		return user_defined_message_type_generator;
	}
	return generator;
}

std::string CxxGenerator::generate_header_file(const MessageSchema &schema) {
	std::ofstream output_file_stream;
	CodeOutput output_file(output_file_stream, schema);
	std::filesystem::path output_path(schema.schema_path);
	output_path.replace_extension(HEADER_FILE_EXT);

	// TODO: this doesn't work with namespaced message names
	const std::string include_guard_name =
		pascal_to_screaming(schema.message_name) + "_NP_HXX";

	output_file_stream.open(output_path);

	std::set<std::string> required_types;
	for (const MessageField &field : schema.fields) {
		required_types.insert(field.type->identifier());
	}

	// clang-format off
	output_file.stream()
	<< "// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND." << std::endl
	<< std::endl
	<< "#ifndef " << include_guard_name << std::endl
	<< "#define " << include_guard_name << std::endl
	<< std::endl
	<< "#include <vector>" << std::endl;
	// clang-format on

	if (required_types.contains(NanoPack::String::IDENTIFIER)) {
		output_file.stream() << "#include <string>" << std::endl;
	}
	if (required_types.contains(NanoPack::Optional::IDENTIFIER)) {
		output_file.stream() << "#include <optional>" << std::endl;
	}
	if (required_types.contains(NanoPack::Map::IDENTIFIER)) {
		output_file.stream() << "#include <unordered_map>" << std::endl;
	}
	// TODO: implement support for including custom message types

	// clang-format off
	output_file.stream()
	<< std::endl
	<< "struct " << schema.message_name << " {" << std::endl
	<< "private:" << std::endl
	<< "  static const int FIELD_COUNT = " << schema.fields.size() << ";" << std::endl
	<< std::endl
	<< "public:" << std::endl
	<< "  static const int32_t TYPE_ID = " << schema.type_id << ";" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.fields) {
		std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator != nullptr) {
			generator->generate_field_declaration(output_file, field);
		}
	}

	// clang-format off
	output_file.stream()
	<< std::endl
	<< std::endl
	<< "  " << schema.message_name << "();" << std::endl
	<< std::endl
	<< "  " << schema.message_name << "(std::vector<uint8_t>::const_iterator begin, int &bytes_read);" << std::endl
	<< std::endl
	<< "  std::vector<uint8_t> data();" << std::endl
	<< "};" << std::endl
	<< std::endl
	<< "#endif" << std::endl;
	// clang-format on

	output_file_stream.close();

	format_code_file(output_path);

	return output_path.filename();
}

void CxxGenerator::generate_code_file(const MessageSchema &schema,
									  const std::string &header_file_name) {
	std::ofstream output_file_stream;
	CodeOutput output_file(output_file_stream, schema);
	std::filesystem::path output_path(schema.schema_path);
	output_path.replace_extension(CODE_FILE_EXT);

	output_file_stream.open(output_path);

	// clang-format off
	output_file_stream
	<< "#include \"" << header_file_name << "\"" << std::endl
	<< "#include <nanopack/reader.hxx>" << std::endl
	<< "#include <nanopack/writer.hxx>" << std::endl
	<< std::endl
	<< schema.message_name << "::" << schema.message_name << "() {}" << std::endl
	<< std::endl
	<< schema.message_name << "::" << schema.message_name << "(std::vector<uint8_t>::const_iterator begin, int &bytes_read) {" << std::endl
	<< "NanoPack::Reader reader(begin);" << std::endl
	<< "int ptr = " << 4 * (schema.fields.size() + 1) << ";" << std::endl
	<< std::endl
	<< "const int32_t type_id = reader.read_type_id();" << std::endl
	<< "if (type_id != " << schema.message_name << "::TYPE_ID) {" << std::endl
	<< "  throw \"incompatible type\";" << std::endl
	<< "}" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.fields) {
		const std::string field_name_snake_case =
			camel_to_snake(field.field_name);

		std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr) {
			// TODO: should probably stop the code gen here since an unsupported
			// 		 type is found.
			continue;
		}

		generator->generate_read_code(output_file, field);
		output_file_stream << std::endl;
	}

	// clang-format off
	output_file_stream
	<< "  bytes_read = ptr;"
	<< "}" << std::endl
	<< std::endl;
	// clang-format on

	// clang-format off
	output_file_stream
	<< "std::vector<uint8_t> " << schema.message_name << "::data() {" << std::endl
	<< "std::vector<uint8_t> buf(sizeof(int32_t) * " << schema.fields.size() + 1 << ");"
	<< "NanoPack::Writer writer(&buf);" << std::endl
	<< std::endl
	<< "writer.write_type_id(" << schema.message_name << "::TYPE_ID);" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.fields) {
		const std::string field_name_snake_case =
			camel_to_snake(field.field_name);

		std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr) {
			// TODO: should probably stop the code gen here since an unsupported
			// 		 type is found.
			continue;
		}

		generator->generate_write_code(output_file, field);
		output_file_stream << std::endl;
	}

	output_file_stream << "return buf;" << std::endl
					   << "}" << std::endl
					   << std::endl;

	output_file_stream.close();

	format_code_file(output_path);
}

void CxxGenerator::format_code_file(const std::filesystem::path &path) {
	const std::string format_cmd =
		"clang-format -i -style=LLVM " + path.string();
	system(format_cmd.c_str());
}
