#include "cxx_bool_generator.hxx"

std::string
CxxBoolGenerator::get_type_declaration(NanoPack::DataType *data_type) {
	return "bool";
}

std::string
CxxBoolGenerator::get_read_size_expression(NanoPack::DataType *data_type,
										   const std::string &var_name) {
	return "sizeof(bool)";
}

void CxxBoolGenerator::generate_constructor_parameter(
	CodeOutput &output, const MessageField &field) {
	output.stream() << "bool " << field.field_name;
}

void CxxBoolGenerator::generate_constructor_field_initializer(
	CodeOutput &output, const MessageField &field) {
	output.stream() << field.field_name << "(" << field.field_name << ")";
}

void CxxBoolGenerator::generate_field_declaration(CodeOutput &output,
												  const MessageField &field) {
	output.stream() << "bool " << field.field_name << ";" << std::endl;
}

void CxxBoolGenerator::generate_read_code(CodeOutput &output,
										  NanoPack::DataType *type,
										  const std::string &var_name) {
	if (output.is_variable_in_scope(var_name)) {
		output.stream() << var_name << " = reader.read_bool(ptr++);"
						<< std::endl
						<< std::endl;
	} else {
		// clang-format off
		output.stream()
		// read boolean value from current buffer read ptr, then move the read ptr
		<< "const bool " << var_name << " = reader.read_bool(ptr++);" << std::endl
		<< std::endl;
		// clang-format on
	}
}

void CxxBoolGenerator::generate_read_code(CodeOutput &output,
										  const MessageField &field) {
	generate_read_code(output, nullptr, field.field_name);
	// clang-format off
	output.stream()
	<< "this->" << field.field_name << " = " << field.field_name << ";" << std::endl
	<< std::endl;
	// clang-format on
}

void CxxBoolGenerator::generate_write_code(CodeOutput &output,
										   NanoPack::DataType *type,
										   const std::string &var_name) {
	// clang-format off
	output.stream()
	<< "writer.append_bool(" << var_name << ");" << std::endl;
	// clang-format on
}

void CxxBoolGenerator::generate_write_code(CodeOutput &output,
										   const MessageField &field) {
	// clang-format off
	output.stream()
	// write the size of the boolean to the size header
	<< "writer.write_field_size(" << field.field_number << ", 1);" << std::endl
	// append the boolean value to the end of the buffer
	<< "writer.append_bool(" << field.field_name << ");" << std::endl
	<< std::endl;
	// clang-format on
}
