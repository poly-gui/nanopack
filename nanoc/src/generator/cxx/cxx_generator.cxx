#include "cxx_generator.hxx"
#include "../../data_type/np_array.hxx"
#include "../../data_type/np_bool.hxx"
#include "../../data_type/np_double.hxx"
#include "../../data_type/np_int32.hxx"
#include "../../data_type/np_int64.hxx"
#include "../../data_type/np_int8.hxx"
#include "../../data_type/np_map.hxx"
#include "../../data_type/np_optional.hxx"
#include "../../data_type/np_string.hxx"
#include "../../string_util/case_conv.hxx"
#include "cxx_array_generator.hxx"
#include "cxx_bool_generator.hxx"
#include "cxx_double_generator.hxx"
#include "cxx_int32_generator.hxx"
#include "cxx_int64_generator.hxx"
#include "cxx_int8_generator.hxx"
#include "cxx_map_generator.hxx"
#include "cxx_message_generator.hxx"
#include "cxx_optional_generator.hxx"
#include "cxx_string_generator.hxx"

#include <filesystem>
#include <fstream>
#include <set>

const std::filesystem::path HEADER_FILE_EXT(".np.hxx");
const std::filesystem::path CODE_FILE_EXT(".np.cxx");

std::string resolve_header_import_path(const MessageSchema &schema_to_import,
									   const MessageSchema &from_schema) {
	std::filesystem::path path_of_schema_to_import(
		schema_to_import.schema_path);
	path_of_schema_to_import.replace_extension(HEADER_FILE_EXT);
	const std::filesystem::path import_path = std::filesystem::relative(
		path_of_schema_to_import, from_schema.schema_path.parent_path());
	return import_path.string();
}

CxxGenerator::CxxGenerator()
	: user_defined_message_type_generator(
		  std::make_shared<CxxMessageGenerator>()),
	  data_type_generator_registry(
		  std::make_shared<DataTypeCodeGeneratorRegistry>()) {
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Bool::IDENTIFIER, std::make_shared<CxxBoolGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int8::IDENTIFIER, std::make_shared<CxxInt8Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int32::IDENTIFIER, std::make_shared<CxxInt32Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int64::IDENTIFIER, std::make_shared<CxxInt64Generator>());
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
	data_type_generator_registry->set_message_generator(
		user_defined_message_type_generator);
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

	const bool has_parent_message = schema.parent_message != nullptr;

	// TODO: this doesn't work with namespaced message names
	const std::string include_guard_name =
		pascal_to_screaming(schema.message_name) + "_NP_HXX";

	output_file_stream.open(output_path);

	std::set<std::string> required_types;
	for (const MessageField &field : schema.declared_fields) {
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

	if (has_parent_message) {
		const std::string import_path =
			resolve_header_import_path(*schema.parent_message, schema);
		output_file.stream()
			<< "#include \"" << import_path << "\"" << std::endl;
	} else {
		output_file.stream() << "#include <nanopack/message.hxx>" << std::endl;
	}

	output_file.stream() << "#include <nanopack/reader.hxx>" << std::endl;

	// clang-format off
	output_file.stream()
	<< std::endl
	<< "struct " << schema.message_name << " : " << (has_parent_message ? schema.parent_message->message_name : "NanoPack::Message") << " {" << std::endl
	<< "  static constexpr int32_t TYPE_ID = " << schema.type_id << ";" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.declared_fields) {
		std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator != nullptr) {
			generator->generate_field_declaration(output_file, field);
		}
	}

	// clang-format off
	output_file.stream()
	<< std::endl
	<< std::endl;
	// clang-format on

	if (schema.is_inherited) {
		// clang-format off
		output_file.stream()
		// factory method for creating subclasses
		<< "  static std::unique_ptr<" << schema.message_name << "> from(std::vector<uint8_t>::const_iterator begin, int &bytes_read);" << std::endl
		<< std::endl;
		// clang-format on
	}

	// clang-format off
	output_file.stream()
	// trivial default constructor definition
	<< "  " << schema.message_name << "() = default;" << std::endl
	<< std::endl;
	// clang-format on

	// constructor with field init params
	if (schema.all_fields.size() == 1) {
		output_file.stream() << "   explicit " << schema.message_name << "(";
	} else {
		output_file.stream() << "  " << schema.message_name << "(";
	}
	size_t i = 0;
	const size_t last_index = schema.all_fields.size() - 1;
	for (const MessageField &field : schema.all_fields) {
		std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator != nullptr) {
			generator->generate_constructor_parameter(output_file, field);
			if (i < last_index) {
				output_file.stream() << ", ";
			}
		}
		i++;
	}
	output_file.stream() << ");" << std::endl << std::endl;

	// clang-format off
	output_file.stream()
	// constructor with byte array param
	<< "  " << schema.message_name << "(std::vector<uint8_t>::const_iterator begin, int &bytes_read);" << std::endl
	<< std::endl
	// constructor with reader param
	<< "  " << schema.message_name << "(const NanoPack::Reader &reader, int &bytes_read);" << std::endl
	<< std::endl
	<< "  [[nodiscard]] std::vector<uint8_t> data() const override;" << std::endl
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

	const bool has_parent_message = schema.parent_message != nullptr;

	output_file_stream.open(output_path);

	// clang-format off
	output_file.stream()
	<< "#include \"" << header_file_name << "\"" << std::endl
	<< "#include <nanopack/reader.hxx>" << std::endl
	<< "#include <nanopack/writer.hxx>" << std::endl
	<< std::endl;
	// clang-format on

	if (schema.is_inherited) {
		for (const std::shared_ptr<MessageSchema> &child_message :
			 schema.child_messages) {
			const std::string import_path =
				resolve_header_import_path(*child_message, schema);
			output_file.stream()
				<< "#include \"" << import_path << "\"" << std::endl;
		}

		// clang-format off
		output_file.stream()
		<< std::endl
		<< "std::unique_ptr<" << schema.message_name << "> " << schema.message_name << "::from(std::vector<uint8_t>::const_iterator begin, int &bytes_read) {" << std::endl
		<< "    const NanoPack::Reader reader(begin);" << std::endl
		<< "    const auto type_id = reader.read_type_id();" << std::endl
		<< "    switch (type_id) {"
		<< "        case TYPE_ID: return std::make_unique<" << schema.message_name << ">(reader, bytes_read);" << std::endl;
		// clang-format on

		for (const std::shared_ptr<MessageSchema> &child_message :
			 schema.child_messages) {
			// clang-format off
			output_file.stream()
			<< "case " << child_message->message_name << "::TYPE_ID: return std::make_unique<" << child_message->message_name << ">(reader, bytes_read);" << std::endl;
			// clang-format on
		}

		// clang-format off
		output_file.stream()
		<< "        default: return nullptr;" << std::endl
		<< "    }" << std::endl
		<< "}" << std::endl
		<< std::endl;
		// clang-format on
	}

	output_file.stream() << schema.message_name << "::" << schema.message_name
						 << "(";
	{
		size_t i = 0;
		const size_t last = schema.all_fields.size() - 1;
		for (const MessageField &field : schema.all_fields) {
			std::shared_ptr<DataTypeCodeGenerator> generator =
				find_generator_for_field(field);
			if (generator == nullptr)
				return;

			generator->generate_constructor_parameter(output_file, field);
			if (i++ < last) {
				output_file.stream() << ", ";
			}
		}
	}
	output_file.stream() << ") : ";

	if (has_parent_message) {
		output_file.stream() << schema.parent_message->message_name << '(';

		size_t i = 0;
		const size_t last = schema.inherited_fields.size() - 1;
		for (const MessageField &field : schema.inherited_fields) {
			output_file.stream() << field.field_name;
			if (i++ < last) {
				output_file.stream() << ", ";
			}
		}

		output_file.stream() << "), ";
	}

	{
		size_t i = 0;
		const size_t last = schema.declared_fields.size() - 1;
		for (const MessageField &field : schema.declared_fields) {
			std::shared_ptr<DataTypeCodeGenerator> generator =
				find_generator_for_field(field);
			if (generator != nullptr) {
				generator->generate_constructor_field_initializer(output_file,
																  field);
				if (i++ < last) {
					output_file.stream() << ", ";
				}
			}
		}
	}
	output_file.stream() << " {}" << std::endl << std::endl;

	// clang-format off
	output_file.stream()
	<< schema.message_name << "::" << schema.message_name << "(const NanoPack::Reader &reader, int &bytes_read)";
	// clang-format on

	if (has_parent_message) {
		output_file.stream()
			<< " : " << schema.parent_message->message_name << "() ";
	}

	// clang-format off
	output_file.stream() << "{" << std::endl
	<< "const auto begin = reader.begin();" << std::endl
	<< "int ptr = " << 4 * (schema.all_fields.size() + 1) << ";" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
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
	<< std::endl
	<< schema.message_name << "::" << schema.message_name << "(std::vector<uint8_t>::const_iterator begin, int &bytes_read) : " << schema.message_name << "(NanoPack::Reader(begin), bytes_read) {}" << std::endl
	<< std::endl;
	// clang-format on

	// clang-format off
	output_file_stream
	<< "std::vector<uint8_t> " << schema.message_name << "::data() const {" << std::endl
	<< "std::vector<uint8_t> buf(sizeof(int32_t) * " << schema.all_fields.size() + 1 << ");"
	<< "NanoPack::Writer writer(&buf);" << std::endl
	<< std::endl
	<< "writer.write_type_id(TYPE_ID);" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
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

void CxxGenerator::generate_message_factory(
	const std::vector<MessageSchema> &all_schemas,
	const std::filesystem::path &output_path) {
	std::ofstream file_stream;

	std::filesystem::path header_file_path(output_path);
	header_file_path.append("nanopack_message_factory.hxx");
	file_stream.open(header_file_path);

	// clang-format off
	file_stream
	<< "#ifndef NANOPACK_MESSAGE_FACTORY_HXX" << std::endl
	<< "#define NANOPACK_MESSAGE_FACTORY_HXX" << std::endl
	<< std::endl
	<< "#include <nanopack/message.hxx>" << std::endl
	<< "#include <memory>" << std::endl
	<< std::endl
	<< "std::unique_ptr<NanoPack::Message> make_nanopack_message(int32_t type_id, std::vector<uint8_t>::const_iterator data_iter, int &bytes_read);" << std::endl
	<< std::endl
	<< "#endif" << std::endl;
	// clang-format on

	file_stream.close();
	file_stream.clear();

	std::filesystem::path code_file_path(output_path);
	code_file_path.append("nanopack_message_factory.cxx");

	file_stream.open(code_file_path);

	file_stream << "#include \"nanopack_message_factory.hxx\"" << std::endl;
	for (const MessageSchema &schema : all_schemas) {
		std::filesystem::path schema_header_path(schema.schema_path);
		schema_header_path.replace_extension(HEADER_FILE_EXT);

		std::filesystem::path import_path =
			relative(schema_header_path, output_path);
		file_stream << "#include \"" << import_path.string() << "\""
					<< std::endl;
	}

	// clang-format off
	file_stream
	<< std::endl
	<< "std::unique_ptr<NanoPack::Message> make_nanopack_message(const int32_t type_id, std::vector<uint8_t>::const_iterator data_iter, int &bytes_read) {" << std::endl
	<< "    switch (type_id) {" << std::endl;
	// clang-format on

	for (const MessageSchema &schema : all_schemas) {
		// clang-format off
		file_stream
		<< "case " << schema.type_id << ": return std::make_unique<" << schema.message_name << ">(data_iter, bytes_read);";
		// clang-format on
	}

	// clang-format off
	file_stream
	<< "    default: return nullptr;" << std::endl
	<< "    }" << std::endl
	<< "}" << std::endl;
	// clang-format on

	file_stream.close();

	format_code_file(header_file_path);
	format_code_file(code_file_path);
}

void CxxGenerator::format_code_file(const std::filesystem::path &path) {
	const std::string format_cmd =
		"clang-format -i -style=LLVM " + path.string();
	system(format_cmd.c_str());
}
