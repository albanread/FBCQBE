#include "runtime_library.h"

namespace fbc {

RuntimeLibrary::RuntimeLibrary(QBEBuilder& builder, TypeManager& typeManager)
    : builder_(builder)
    , typeManager_(typeManager)
{
}

// === Print/Output ===

void RuntimeLibrary::emitPrintInt(const std::string& value) {
    emitRuntimeCallVoid("basic_print_int", "w " + value);
}

void RuntimeLibrary::emitPrintFloat(const std::string& value) {
    emitRuntimeCallVoid("basic_print_float", "s " + value);
}

void RuntimeLibrary::emitPrintDouble(const std::string& value) {
    emitRuntimeCallVoid("basic_print_double", "d " + value);
}

void RuntimeLibrary::emitPrintString(const std::string& stringPtr) {
    emitRuntimeCallVoid("basic_print_string", "l " + stringPtr);
}

void RuntimeLibrary::emitPrintNewline() {
    emitRuntimeCallVoid("basic_print_newline", "");
}

void RuntimeLibrary::emitPrintTab() {
    emitRuntimeCallVoid("basic_print_tab", "");
}

// === String Operations ===

std::string RuntimeLibrary::emitStringConcat(const std::string& left, const std::string& right) {
    return emitRuntimeCall("basic_string_concat", "l", "l " + left + ", l " + right);
}

std::string RuntimeLibrary::emitStringLen(const std::string& stringPtr) {
    // BasicString struct layout:
    //   offset 0: char* data
    //   offset 8: size_t length (8 bytes on 64-bit)
    //   offset 16: size_t capacity
    //   offset 24: int32_t refcount
    // We want to load the length field at offset 8
    
    std::string lengthAddr = builder_.newTemp();
    builder_.emitBinary(lengthAddr, "l", "add", stringPtr, "8");
    
    std::string lengthVal = builder_.newTemp();
    builder_.emitLoad(lengthVal, "l", lengthAddr);  // Load size_t (64-bit)
    
    // Truncate to 32-bit for BASIC INTEGER compatibility
    std::string result = builder_.newTemp();
    builder_.emitTrunc(result, "w", lengthVal);
    
    return result;
}

std::string RuntimeLibrary::emitChr(const std::string& charCode) {
    return emitRuntimeCall("basic_chr", "l", "w " + charCode);
}

std::string RuntimeLibrary::emitAsc(const std::string& stringPtr) {
    return emitRuntimeCall("basic_asc", "w", "l " + stringPtr);
}

std::string RuntimeLibrary::emitMid(const std::string& stringPtr, const std::string& start, 
                                   const std::string& length) {
    if (length.empty()) {
        // MID$(s$, start) - to end of string
        return emitRuntimeCall("basic_mid_toend", "l", "l " + stringPtr + ", w " + start);
    } else {
        // MID$(s$, start, length)
        return emitRuntimeCall("basic_mid", "l", 
            "l " + stringPtr + ", w " + start + ", w " + length);
    }
}

std::string RuntimeLibrary::emitLeft(const std::string& stringPtr, const std::string& count) {
    return emitRuntimeCall("basic_left", "l", "l " + stringPtr + ", w " + count);
}

std::string RuntimeLibrary::emitRight(const std::string& stringPtr, const std::string& count) {
    return emitRuntimeCall("basic_right", "l", "l " + stringPtr + ", w " + count);
}

std::string RuntimeLibrary::emitStringCompare(const std::string& left, const std::string& right) {
    return emitRuntimeCall("basic_string_compare", "w", "l " + left + ", l " + right);
}

void RuntimeLibrary::emitStringAssign(const std::string& dest, const std::string& src) {
    emitRuntimeCallVoid("basic_string_assign", "l " + dest + ", l " + src);
}

std::string RuntimeLibrary::emitStringLiteral(const std::string& stringConstant) {
    return emitRuntimeCall("str_new", "l", "l $" + stringConstant);
}

// === Array Operations ===

std::string RuntimeLibrary::emitArrayAccess(const std::string& arrayBase, 
                                           const std::string& index,
                                           BasicType elementType) {
    // Calculate offset: base + (index * elementSize)
    int elementSize = typeManager_.getTypeSize(elementType);
    
    std::string offsetTemp = builder_.newTemp();
    builder_.emitBinary(offsetTemp, "l", "mul", index, std::to_string(elementSize));
    
    std::string addrTemp = builder_.newTemp();
    builder_.emitBinary(addrTemp, "l", "add", arrayBase, offsetTemp);
    
    return addrTemp;
}

void RuntimeLibrary::emitArrayBoundsCheck(const std::string& index, 
                                         const std::string& lowerBound,
                                         const std::string& upperBound) {
    emitRuntimeCallVoid("basic_array_bounds_check", 
        "w " + index + ", w " + lowerBound + ", w " + upperBound);
}

std::string RuntimeLibrary::emitArrayAlloc(BasicType elementType, const std::string& totalSize) {
    int elementSize = typeManager_.getTypeSize(elementType);
    
    std::string byteSizeTemp = builder_.newTemp();
    builder_.emitBinary(byteSizeTemp, "l", "mul", totalSize, std::to_string(elementSize));
    
    return emitRuntimeCall("basic_alloc_array", "l", "l " + byteSizeTemp);
}

// === Math Functions ===

std::string RuntimeLibrary::emitAbs(const std::string& value, BasicType valueType) {
    if (typeManager_.isIntegral(valueType)) {
        return emitRuntimeCall("basic_abs_int", "w", "w " + value);
    } else if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("basic_abs_float", "s", "s " + value);
    } else {
        return emitRuntimeCall("basic_abs_double", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitSqr(const std::string& value, BasicType valueType) {
    if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("sqrtf", "s", "s " + value);
    } else {
        return emitRuntimeCall("sqrt", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitSin(const std::string& value, BasicType valueType) {
    if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("sinf", "s", "s " + value);
    } else {
        return emitRuntimeCall("sin", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitCos(const std::string& value, BasicType valueType) {
    if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("cosf", "s", "s " + value);
    } else {
        return emitRuntimeCall("cos", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitTan(const std::string& value, BasicType valueType) {
    if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("tanf", "s", "s " + value);
    } else {
        return emitRuntimeCall("tan", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitInt(const std::string& value, BasicType valueType) {
    if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("floorf", "s", "s " + value);
    } else {
        return emitRuntimeCall("floor", "d", "d " + value);
    }
}

std::string RuntimeLibrary::emitRnd() {
    return emitRuntimeCall("basic_rnd", "s", "");
}

std::string RuntimeLibrary::emitTimer() {
    return emitRuntimeCall("basic_timer", "d", "");
}

// === Input ===

void RuntimeLibrary::emitInputInt(const std::string& dest) {
    emitRuntimeCallVoid("basic_input_int", "l " + dest);
}

void RuntimeLibrary::emitInputFloat(const std::string& dest) {
    emitRuntimeCallVoid("basic_input_float", "l " + dest);
}

void RuntimeLibrary::emitInputDouble(const std::string& dest) {
    emitRuntimeCallVoid("basic_input_double", "l " + dest);
}

void RuntimeLibrary::emitInputString(const std::string& dest) {
    emitRuntimeCallVoid("basic_input_string", "l " + dest);
}

// === Memory/Conversion ===

std::string RuntimeLibrary::emitStr(const std::string& value, BasicType valueType) {
    if (typeManager_.isIntegral(valueType)) {
        return emitRuntimeCall("basic_str_int", "l", "w " + value);
    } else if (valueType == BasicType::SINGLE) {
        return emitRuntimeCall("basic_str_float", "l", "s " + value);
    } else {
        return emitRuntimeCall("basic_str_double", "l", "d " + value);
    }
}

std::string RuntimeLibrary::emitVal(const std::string& stringPtr) {
    return emitRuntimeCall("basic_val", "d", "l " + stringPtr);
}

// === Control Flow Helpers ===

void RuntimeLibrary::emitEnd() {
    emitRuntimeCallVoid("exit", "w 0");
    // QBE requires a terminator after every call, even if the call doesn't return
    builder_.emitReturn("0");
}

void RuntimeLibrary::emitRuntimeError(int errorCode, const std::string& errorMsg) {
    emitRuntimeCallVoid("basic_runtime_error", 
        "w " + std::to_string(errorCode) + ", l " + errorMsg);
}

// === Private Helpers ===

std::string RuntimeLibrary::emitRuntimeCall(const std::string& funcName, 
                                           const std::string& returnType,
                                           const std::string& args) {
    std::string result = builder_.newTemp();
    builder_.emitCall(result, returnType, funcName, args);
    return result;
}

void RuntimeLibrary::emitRuntimeCallVoid(const std::string& funcName, 
                                        const std::string& args) {
    builder_.emitCall("", "", funcName, args);
}

} // namespace fbc