#include "ts_generator.hxx"

#include "../../data_type/np_array.hxx"
#include "../../data_type/np_bool.hxx"
#include "../../data_type/np_int32.hxx"
#include "../../data_type/np_int64.hxx"
#include "../../data_type/np_int8.hxx"
#include "../../data_type/np_map.hxx"
#include "../../data_type/np_optional.hxx"
#include "../../data_type/np_string.hxx"
#include "../../string_util/case_conv.hxx"
#include "ts_array_generator.hxx"
#include "ts_bool_generator.hxx"
#include "ts_int32_generator.hxx"
#include "ts_int64_generator.hxx"
#include "ts_int8_generator.hxx"
#include "ts_map_generator.hxx"
#include "ts_message_generator.hxx"
#include "ts_optional_generator.hxx"
#include "ts_string_generator.hxx"

#include <fstream>

const std::filesystem::path CODE_FILE_EXT(".np.ts");

std::string resolve_ts_import_path(const MessageSchema &schema_to_import,
								   const MessageSchema &from_schema) {
	std::filesystem::path path_of_schema_to_import(
		schema_to_import.schema_path);
	path_of_schema_to_import.replace_extension(".np.js");
	const std::filesystem::path import_path = std::filesystem::relative(
		path_of_schema_to_import, from_schema.schema_path.parent_path());
	std::string import_path_string = import_path.string();

	if (import_path_string[0] != '.') {
		import_path_string.insert(0, "./");
	}

	return import_path_string;
}

std::string resolve_ts_import_path(const std::filesystem::path &path_to_import,
								   const std::filesystem::path &from_path) {
	const std::filesystem::path import_path =
		relative(path_to_import, from_path);
	std::string import_path_string = import_path.string();
	if (import_path_string[0] != '.') {
		import_path_string.insert(0, "./");
	}
	return import_path_string;
}

void format_file(const std::filesystem::path &path) {
	const std::string format_cmd = "npx prettier " + path.string() + " --write";
	system(format_cmd.c_str());
}

TsGenerator::TsGenerator()
	: user_defined_message_type_generator(
		  std::make_shared<TsMessageGenerator>()),
	  data_type_generator_registry(
		  std::make_shared<DataTypeCodeGeneratorRegistry>()) {
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Bool::IDENTIFIER, std::make_shared<TsBoolGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int8::IDENTIFIER, std::make_shared<TsInt8Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int32::IDENTIFIER, std::make_shared<TsInt32Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Int64::IDENTIFIER, std::make_shared<TsInt64Generator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::String::IDENTIFIER, std::make_shared<TsStringGenerator>());
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Array::IDENTIFIER,
		std::make_shared<TsArrayGenerator>(data_type_generator_registry));
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Map::IDENTIFIER,
		std::make_shared<TsMapGenerator>(data_type_generator_registry));
	data_type_generator_registry->add_generator_for_type(
		NanoPack::Optional::IDENTIFIER,
		std::make_shared<TsOptionalGenerator>(data_type_generator_registry));
	data_type_generator_registry->set_message_generator(
		user_defined_message_type_generator);
}

void TsGenerator::generate_for_schema(const MessageSchema &schema) {
	std::ofstream output_file_stream;
	CodeOutput code_output(output_file_stream, schema);
	std::filesystem::path output_path(schema.schema_path);
	output_path.replace_filename(snake_to_kebab(output_path.filename()))
		.replace_extension(CODE_FILE_EXT);

	output_file_stream.open(output_path);

	const bool has_parent_message = schema.parent_message != nullptr;

	// clang-format off
	code_output.stream()
	<< "// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND." << std::endl
	<< std::endl
	<< "import { NanoBufReader, NanoBufWriter " << (has_parent_message ? "" : ", type NanoPackMessage") << "} from \"nanopack\"" << std::endl
	<< std::endl;
	// clang-format on

	for (const std::shared_ptr<MessageSchema> &imported_message :
		 schema.imported_messages) {
		// import class definition of all imported messages.
		// if the imported message is inherited, then its factory function needs
		// to be imported as well for use when reading the field that stores the
		// imported message.

		// clang-format off
		code_output.stream()
		<< "import { " << imported_message->message_name << " } from \"" << resolve_ts_import_path(*imported_message, schema) << "\";" << std::endl;
		// clang-format on
	}

	for (const MessageField &field : schema.all_fields) {
		if (!field.type->is_user_defined()) {
			continue;
		}

		const std::shared_ptr<MessageSchema> type_schema =
			schema.find_imported_schema(field.type->identifier());
		if (type_schema->is_inherited) {
			std::filesystem::path factory_file_path(type_schema->schema_path);
			factory_file_path
				.replace_filename("make-" +
								  pascal_to_kebab(type_schema->message_name))
				.replace_extension(".np.js");

			// clang-format off
			code_output.stream()
			<< "import { make" << type_schema->message_name << " } from \"" << resolve_ts_import_path(factory_file_path, schema.schema_path.parent_path()) << "\";" << std::endl;
			// clang-format on
		}
	}

	// clang-format off
	code_output.stream()
	<< std::endl
	<< "class " << schema.message_name << (has_parent_message ? " extends " + schema.parent_message->message_name : " implements NanoPackMessage") << "{" << std::endl
	<< "    public static TYPE_ID = " << schema.type_id << ";" << std::endl
	<< std::endl
	<< "    constructor(";
	// clang-format on

	for (const MessageField &field : schema.inherited_fields) {
		const std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr)
			continue;

		generator->generate_constructor_parameter(code_output, field);
		code_output.stream() << ", ";
	}

	for (const MessageField &field : schema.declared_fields) {
		const std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr)
			continue;

		code_output.stream() << "public ";
		generator->generate_constructor_parameter(code_output, field);
		code_output.stream() << ", ";
	}

	if (has_parent_message) {
		// clang-format off
		code_output.stream()
		<< ") {" << std::endl
		<< "    super(";
		// clang-format on
		for (const MessageField &field : schema.inherited_fields) {
			code_output.stream()
				<< snake_to_camel(field.field_name) << "," << std::endl;
		}
		code_output.stream() << ");" << std::endl << "}" << std::endl;
	} else {
		code_output.stream() << ") {}" << std::endl;
	}

	// clang-format off
	code_output.stream()
	<< std::endl
	<< "public static fromBytes(bytes: Uint8Array): { bytesRead: number, result: " << schema.message_name << " } | null {" << std::endl
	<< "    const reader = new NanoBufReader(bytes);" << std::endl
	<< "    return " << schema.message_name << ".fromReader(reader);" << std::endl
	<< "}" << std::endl
	<< std::endl
	<< "public static fromReader(reader: NanoBufReader): { bytesRead: number, result: " << schema.message_name << " } | null {" << std::endl
	<< "    let ptr = " << (schema.all_fields.size() + 1) * 4 << ";" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
		const std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr)
			continue;

		generator->generate_read_code(code_output, field);
		code_output.stream() << std::endl;
	}

	// clang-format off
	code_output.stream()
	<< "return { bytesRead: ptr, result: new " << schema.message_name << "(";
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
		code_output.stream() << snake_to_camel(field.field_name) << ", ";
	}

	// clang-format off
	code_output.stream()
	<< ") };" << std::endl
	<< "}" << std::endl
	<< std::endl
	<< (has_parent_message ? "override " : "") << "public get typeId(): number {" << std::endl
	<< "    return " << schema.type_id << ";" << std::endl
	<< "}" << std::endl
	<< std::endl
	<< (has_parent_message ? "override " : "") << "public bytes(): Uint8Array {" << std::endl;
	// clang-format on

	const auto writer_initial_size = (schema.all_fields.size() + 1) * 4;

	// clang-format off
	code_output.stream()
	<< "    const writer = new NanoBufWriter(" << writer_initial_size << ");" << std::endl
	<< "    writer.writeTypeId(" << schema.type_id << ");" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
		const std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr)
			continue;

		generator->generate_write_code(code_output, field);
		code_output.stream() << std::endl;
	}

	// clang-format off
	code_output.stream()
	<< "    return writer.bytes;" << std::endl
	<< "}" << std::endl // bytes()
	<< std::endl
	<< (has_parent_message ? "override " : "") << "public bytesWithLengthPrefix(): Uint8Array {" << std::endl
	<< "    const writer = new NanoBufWriter(" << writer_initial_size + 4 << ", true);" << std::endl
	<< "    writer.writeTypeId(" << schema.type_id << ");" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageField &field : schema.all_fields) {
		const std::shared_ptr<DataTypeCodeGenerator> generator =
			find_generator_for_field(field);
		if (generator == nullptr)
			continue;

		generator->generate_write_code(code_output, field);
		code_output.stream() << std::endl;
	}

	// clang-format off
	code_output.stream()
	<< "    writer.writeLengthPrefix(writer.currentSize - 4)" << std::endl
	<< std::endl
	<< "    return writer.bytes;" << std::endl
	<< "}" << std::endl // bytesWithLengthPrefix()
	<< "}" << std::endl // class
	<< std::endl
	<< "export { " << schema.message_name << " };" << std::endl;
	// clang-format on

	output_file_stream.close();

	format_file(output_path);

	if (schema.is_inherited) {
		generate_factory_file(schema);
	}
}

void TsGenerator::generate_message_factory(
	const std::vector<MessageSchema> &all_schemas,
	const std::filesystem::path &output_path) {
	std::ofstream file_stream;
	std::filesystem::path output_file_path(output_path);
	output_file_path.append("message-factory.ts");

	file_stream.open(output_file_path);

	// clang-format off
	file_stream
	<< "import type { NanoPackMessage } from \"nanopack\";" << std::endl
	<< std::endl;
	// clang-format on

	for (const MessageSchema &schema : all_schemas) {
		std::filesystem::path schema_path(schema.schema_path);
		schema_path.replace_extension(".np.js");

		std::filesystem::path import_path = relative(schema_path, output_path);
		std::string import_path_string = import_path.string();

		if (import_path_string[0] != '.') {
			import_path_string.insert(0, "./");
		}

		file_stream << "import { " << schema.message_name << " } from \""
					<< import_path_string << "\";" << std::endl;
	}

	// clang-format off
	file_stream
	<< std::endl
	<< "function makeNanoPackMessage(bytes: Uint8Array, typeId: number): { bytesRead: number, result: NanoPackMessage } | null {" << std::endl
	<< "    switch (typeId) {" << std::endl;
	// clang-format on

	for (const MessageSchema &schema : all_schemas) {
		file_stream << "case " << schema.type_id << ": return "
					<< schema.message_name << ".fromBytes(bytes);" << std::endl;
	}

	// clang-format off
	file_stream
	<< "    default: return null;" << std::endl
	<< "    }" << std::endl
	<< "}" << std::endl
	<< std::endl
	<< "export { makeNanoPackMessage };" << std::endl;
	// clang-format on

	const std::string format_cmd =
		"npx prettier " + output_file_path.string() + " --write";
	system(format_cmd.c_str());
}

std::shared_ptr<DataTypeCodeGenerator>
TsGenerator::find_generator_for_field(const MessageField &field) const {
	auto generator =
		data_type_generator_registry->find_generator_for_type(field.type.get());
	if (generator == nullptr)
		return user_defined_message_type_generator;
	return generator;
}

void TsGenerator::generate_factory_file(const MessageSchema &schema) {
	std::ofstream file_stream;
	std::filesystem::path output_path(schema.schema_path);
	output_path
		.replace_filename("make-" + snake_to_kebab(output_path.filename()))
		.replace_extension(CODE_FILE_EXT);

	file_stream.open(output_path);

	// clang-format off
	file_stream
	<< "// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND." << std::endl
	<< std::endl
	<< "import { NanoBufReader } from \"nanopack\";" << std::endl
	<< "import { " << schema.message_name << " } from \"./" << pascal_to_kebab(schema.message_name) << ".np.js\";" << std::endl;
	// clang-format on

	for (const std::shared_ptr<MessageSchema> &child_message :
		 schema.child_messages) {
		// clang-format off
		file_stream
		<< "import { " << child_message->message_name << " } from \"./" << resolve_ts_import_path(*child_message, schema) << "\";" << std::endl;
		// clang-format on
	}

	// clang-format off
	file_stream
	<< std::endl
	<< "function make" << schema.message_name << "(bytes: Uint8Array) {" << std::endl
	<< "    const reader = new NanoBufReader(bytes);" << std::endl
	<< "    switch (reader.readTypeId()) {" << std::endl
	<< "    case " << schema.type_id << ": return " << schema.message_name << ".fromReader(reader);" << std::endl;
	// clang-format on

	for (const std::shared_ptr<MessageSchema> &child_schema :
		 schema.child_messages) {
		// clang-format off
		file_stream
		<< "case " << child_schema->type_id << ": return " << child_schema->message_name << ".fromReader(reader);" << std::endl;
		// clang-format on
	}

	// clang-format off
	file_stream
	<< "    default: return null;" << std::endl
	<< "    }" << std::endl // switch
	<< "}" << std::endl // function
	<< std::endl
	<< "export { make" <<schema.message_name << " };" << std::endl;
	// clang-format on

	file_stream.close();

	format_file(output_path);
}
