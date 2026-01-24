//
// qbe_codegen_runtime.cpp
// FasterBASIC QBE Code Generator - Runtime Library Interface
//
// This file contains wrappers for calling runtime library functions:
// - I/O operations (print, input)
// - String operations (concat, compare, substring)
// - Array operations (create, get, set)
// - Type conversions (int/double/string)
// - Math operations
//

#include "../fasterbasic_qbe_codegen.h"

namespace FasterBASIC {

// =============================================================================
// I/O Operations - Print
// =============================================================================

void QBECodeGenerator::emitPrintValue(const std::string& value, VariableType type) {
    if (type == VariableType::STRING) {
        emit("    call $basic_print_string(l " + value + ")\n");
    } else if (type == VariableType::INT) {
        emit("    call $basic_print_int(w " + value + ")\n");
    } else if (type == VariableType::DOUBLE) {
        emit("    call $basic_print_double(d " + value + ")\n");
    } else if (type == VariableType::FLOAT) {
        emit("    call $basic_print_float(d " + value + ")\n");
    } else {
        // Default to int
        emit("    call $basic_print_int(w " + value + ")\n");
    }
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitPrintNewline() {
    emit("    call $basic_print_newline()\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitPrintTab() {
    emit("    call $basic_print_tab()\n");
    m_stats.instructionsGenerated++;
}

// =============================================================================
// I/O Operations - Input
// =============================================================================

std::string QBECodeGenerator::emitInputString() {
    std::string temp = allocTemp("l");
    emit("    " + temp + " =l call $basic_input_string()\n");
    m_stats.instructionsGenerated++;
    return temp;
}

std::string QBECodeGenerator::emitInputInt() {
    std::string temp = allocTemp("w");
    emit("    " + temp + " =w call $basic_input_int()\n");
    m_stats.instructionsGenerated++;
    return temp;
}

std::string QBECodeGenerator::emitInputDouble() {
    std::string temp = allocTemp("d");
    emit("    " + temp + " =d call $basic_input_double()\n");
    m_stats.instructionsGenerated++;
    return temp;
}

// =============================================================================
// String Operations
// =============================================================================

std::string QBECodeGenerator::emitStringConstant(const std::string& str) {
    // Check if we already have this string
    auto it = m_stringLiterals.find(str);
    if (it != m_stringLiterals.end()) {
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l copy $str." + std::to_string(it->second) + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Add new string to data section
    int strId = m_dataStrings.size();
    m_dataStrings.push_back(str);
    m_stringLiterals[str] = strId;
    
    std::string temp = allocTemp("l");
    emit("    " + temp + " =l copy $str." + std::to_string(strId) + "\n");
    m_stats.instructionsGenerated++;
    
    return temp;
}

std::string QBECodeGenerator::emitStringConcat(const std::string& left, const std::string& right) {
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $str_concat(l " + left + ", l " + right + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitStringCompare(const std::string& left, const std::string& right) {
    std::string result = allocTemp("w");
    emit("    " + result + " =w call $str_compare(l " + left + ", l " + right + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitStringLength(const std::string& str) {
    std::string result = allocTemp("w");
    emit("    " + result + " =w call $str_length(l " + str + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitStringSubstr(const std::string& str, 
                                               const std::string& start, 
                                               const std::string& length) {
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $str_substr(l " + str + ", w " + start + ", w " + length + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

// =============================================================================
// Array Operations
// =============================================================================

std::string QBECodeGenerator::emitArrayCreate(const std::string& arrayName, 
                                              const std::vector<std::string>& bounds) {
    std::string temp = allocTemp("l");
    
    // Call runtime array creation
    // First argument is number of dimensions
    emit("    " + temp + " =l call $array_create(w " + std::to_string(bounds.size()));
    
    // Then dimension sizes
    for (const auto& bound : bounds) {
        emit(", w " + bound);
    }
    
    emit(")\n");
    
    m_stats.instructionsGenerated++;
    return temp;
}

std::string QBECodeGenerator::emitArrayGet(const std::string& arrayName, 
                                           const std::vector<std::string>& indices) {
    std::string arrayRef = getArrayRef(arrayName);
    std::string temp = allocTemp("w");
    
    emit("    " + temp + " =w call $array_get(l " + arrayRef);
    
    for (const auto& index : indices) {
        emit(", w " + index);
    }
    
    emit(")\n");
    
    m_stats.instructionsGenerated++;
    return temp;
}

void QBECodeGenerator::emitArrayStore(const std::string& arrayName, 
                                      const std::vector<std::string>& indices,
                                      const std::string& value) {
    std::string arrayRef = getArrayRef(arrayName);
    
    emit("    call $array_set(l " + arrayRef);
    
    for (const auto& index : indices) {
        emit(", w " + index);
    }
    
    emit(", w " + value + ")\n");
    
    m_stats.instructionsGenerated++;
}

// =============================================================================
// Type Conversion Operations
// =============================================================================

std::string QBECodeGenerator::emitIntToString(const std::string& value) {
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $int_to_str(w " + value + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitDoubleToString(const std::string& value) {
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $double_to_str(d " + value + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitStringToInt(const std::string& value) {
    std::string result = allocTemp("w");
    emit("    " + result + " =w call $str_to_int(l " + value + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitStringToDouble(const std::string& value) {
    std::string result = allocTemp("d");
    emit("    " + result + " =d call $str_to_double(l " + value + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitIntToDouble(const std::string& value) {
    std::string result = allocTemp("d");
    emit("    " + result + " =d extsw " + value + "\n");
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitDoubleToInt(const std::string& value) {
    std::string result = allocTemp("w");
    emit("    " + result + " =w dtosi " + value + "\n");
    m_stats.instructionsGenerated++;
    return result;
}

// =============================================================================
// Math Operations
// =============================================================================

std::string QBECodeGenerator::emitMathFunction(const std::string& funcName, 
                                               const std::vector<std::string>& args) {
    std::string result = allocTemp("d");
    
    // Build function call
    emit("    " + result + " =d call $basic_" + funcName + "(");
    
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) emit(", ");
        emit("d " + args[i]);
    }
    
    emit(")\n");
    
    m_stats.instructionsGenerated++;
    return result;
}

std::string QBECodeGenerator::emitAbs(const std::string& value) {
    return emitMathFunction("abs", {value});
}

std::string QBECodeGenerator::emitSqrt(const std::string& value) {
    return emitMathFunction("sqrt", {value});
}

std::string QBECodeGenerator::emitSin(const std::string& value) {
    return emitMathFunction("sin", {value});
}

std::string QBECodeGenerator::emitCos(const std::string& value) {
    return emitMathFunction("cos", {value});
}

std::string QBECodeGenerator::emitTan(const std::string& value) {
    return emitMathFunction("tan", {value});
}

std::string QBECodeGenerator::emitPow(const std::string& base, const std::string& exp) {
    return emitMathFunction("pow", {base, exp});
}

std::string QBECodeGenerator::emitRnd() {
    std::string result = allocTemp("d");
    emit("    " + result + " =d call $basic_rnd()\n");
    m_stats.instructionsGenerated++;
    return result;
}

// =============================================================================
// File I/O Operations
// =============================================================================

void QBECodeGenerator::emitFileOpen(const std::string& filename, 
                                    const std::string& mode,
                                    const std::string& fileNum) {
    emit("    call $file_open(l " + filename + ", w " + mode + ", w " + fileNum + ")\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitFileClose(const std::string& fileNum) {
    emit("    call $file_close(w " + fileNum + ")\n");
    m_stats.instructionsGenerated++;
}

std::string QBECodeGenerator::emitFileRead(const std::string& fileNum) {
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $file_read_line(w " + fileNum + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

void QBECodeGenerator::emitFileWrite(const std::string& fileNum, const std::string& data) {
    emit("    call $file_write_line(w " + fileNum + ", l " + data + ")\n");
    m_stats.instructionsGenerated++;
}

std::string QBECodeGenerator::emitFileEof(const std::string& fileNum) {
    std::string result = allocTemp("w");
    emit("    " + result + " =w call $file_eof(w " + fileNum + ")\n");
    m_stats.instructionsGenerated++;
    return result;
}

} // namespace FasterBASIC