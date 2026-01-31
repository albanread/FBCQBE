# DATA, READ, and RESTORE Implementation Plan

## Current Status

**Status**: ❌ Disabled - Data preprocessing currently commented out in fbc_qbe.cpp

The existing DATA preprocessor infrastructure exists but is not integrated with the QBE code generator.

---

## Overview

The DATA/READ/RESTORE system allows BASIC programs to embed read-only data within the program and access it sequentially:

```basic
10 DATA 100, 200, "Hello", 3.14
20 READ A%, B%, C$, D
30 PRINT A%, B%, C$, D
40 RESTORE
50 READ X%, Y%
60 PRINT X%, Y%
```

---

## Existing Infrastructure

### 1. Data Preprocessor ✅

**Files**:
- `fasterbasic_data_preprocessor.h`
- `fasterbasic_data_preprocessor.cpp`

**Capabilities**:
- Extracts DATA statements from source
- Parses typed values (int, double, string)
- Tracks restore points by line number and label
- Removes DATA lines from source before parsing

**Key Function**:
```cpp
DataPreprocessorResult process(const std::string& source);

struct DataPreprocessorResult {
    std::vector<DataValue> values;                    // All DATA values
    std::map<int, size_t> lineRestorePoints;          // Line → index
    std::map<std::string, size_t> labelRestorePoints; // Label → index
    std::string cleanedSource;                        // Source without DATA
};
```

### 2. AST Nodes ✅

**File**: `fasterbasic_ast.h`

```cpp
class DataStatement : public Statement {
    std::vector<std::string> values;
};

class ReadStatement : public Statement {
    std::vector<std::string> variables;
};

class RestoreStatement : public Statement {
    int lineNumber;       // 0 if not specified
    std::string label;    // Empty if not specified
    bool isLabel;
};
```

### 3. Semantic Analysis ✅

**File**: `fasterbasic_semantic.cpp`

Already handles DATA/READ/RESTORE in symbol table:
- Lines 791, 808: DATA statement processing
- Lines 1173, 1176: READ/RESTORE statement handling

---

## QBE Implementation Strategy

### Design Philosophy

**Read-only data segment** - DATA values stored in QBE data section, accessed via runtime pointer

```qbe
# Data Section
data $__basic_data = align 8 {
    w 100,           # INT
    w 200,           # INT
    l $str_0,        # STRING pointer
    d d_3.14,        # DOUBLE
    # ... more values
}

# Type tags (parallel array)
data $__basic_data_types = align 1 {
    b 0,  # 0 = INT
    b 0,  # 0 = INT
    b 2,  # 2 = STRING
    b 1,  # 1 = DOUBLE
}

# String constants referenced by data
data $str_0 = { b "Hello", b 0 }

# Global read pointer (starts at 0)
export data $__basic_data_ptr = align 8 { l 0 }
```

### Memory Layout

Each data value needs:
1. **Value storage** (8 bytes max)
2. **Type tag** (1 byte)

Total: ~9 bytes per value (with alignment)

**Type encoding**:
- 0 = INT (32-bit)
- 1 = DOUBLE (64-bit)
- 2 = STRING (pointer)

---

## Implementation Phases

### Phase 1: Enable Data Preprocessing ✅ (Already exists)

**File**: `fbc_qbe.cpp` line 137-140

```cpp
// Currently disabled - need to re-enable:
DataPreprocessor preprocessor;
DataPreprocessorResult dataResult = preprocessor.process(source);
source = dataResult.cleanedSource;

// Store dataResult for later use in code generation
```

**Action**: Pass `dataResult` to code generator

---

### Phase 2: QBE Data Section Generation

**File**: `qbe_codegen_main.cpp`

#### 2.1 Add Data Storage to Code Generator

```cpp
class QBECodeGenerator {
private:
    struct DataEntry {
        DataValue value;      // int, double, or string
        std::string label;    // Optional label for RESTORE
        int lineNumber;       // Optional line number for RESTORE
    };
    
    std::vector<DataEntry> m_dataValues;
    std::map<int, size_t> m_lineRestorePoints;
    std::map<std::string, size_t> m_labelRestorePoints;
    
public:
    void setDataValues(const DataPreprocessorResult& dataResult);
    void emitDataSection();
};
```

#### 2.2 Emit Data Section

```cpp
void QBECodeGenerator::emitDataSection() {
    if (m_dataValues.empty()) return;
    
    emit("# DATA values\n");
    emit("data $__basic_data = align 8 {\n");
    
    for (const auto& entry : m_dataValues) {
        if (std::holds_alternative<int>(entry.value)) {
            int val = std::get<int>(entry.value);
            emit("    w " + std::to_string(val) + ",\n");
        } else if (std::holds_alternative<double>(entry.value)) {
            double val = std::get<double>(entry.value);
            emit("    d d_" + formatDouble(val) + ",\n");
        } else {
            // String - emit pointer to string constant
            std::string str = std::get<std::string>(entry.value);
            std::string strLabel = allocStringConstant(str);
            emit("    l " + strLabel + ",\n");
        }
    }
    
    emit("}\n\n");
    
    // Emit type tags
    emit("data $__basic_data_types = align 1 {\n");
    for (const auto& entry : m_dataValues) {
        if (std::holds_alternative<int>(entry.value)) {
            emit("    b 0,  # INT\n");
        } else if (std::holds_alternative<double>(entry.value)) {
            emit("    b 1,  # DOUBLE\n");
        } else {
            emit("    b 2,  # STRING\n");
        }
    }
    emit("}\n\n");
    
    // Emit read pointer (global variable)
    emit("export data $__basic_data_ptr = align 8 { l 0 }\n\n");
}
```

---

### Phase 3: Runtime Support Functions

**File**: Create `runtime/basic_data.c`

```c
#include <stdint.h>
#include <string.h>

// External symbols from generated code
extern int64_t __basic_data[];        // Array of data values
extern uint8_t __basic_data_types[];  // Parallel array of type tags
extern int64_t __basic_data_ptr;      // Current read position

// Type tags
#define DATA_TYPE_INT    0
#define DATA_TYPE_DOUBLE 1
#define DATA_TYPE_STRING 2

// READ functions - called by generated code
int32_t basic_read_int() {
    size_t idx = (size_t)__basic_data_ptr;
    if (__basic_data_types[idx] != DATA_TYPE_INT) {
        // Type mismatch - could error or coerce
        // For now, coerce
        if (__basic_data_types[idx] == DATA_TYPE_DOUBLE) {
            double* dptr = (double*)&__basic_data[idx];
            __basic_data_ptr++;
            return (int32_t)*dptr;
        }
    }
    int32_t value = (int32_t)__basic_data[idx];
    __basic_data_ptr++;
    return value;
}

double basic_read_double() {
    size_t idx = (size_t)__basic_data_ptr;
    if (__basic_data_types[idx] != DATA_TYPE_DOUBLE) {
        // Type mismatch - coerce
        if (__basic_data_types[idx] == DATA_TYPE_INT) {
            int32_t ival = (int32_t)__basic_data[idx];
            __basic_data_ptr++;
            return (double)ival;
        }
    }
    double* dptr = (double*)&__basic_data[idx];
    __basic_data_ptr++;
    return *dptr;
}

const char* basic_read_string() {
    size_t idx = (size_t)__basic_data_ptr;
    if (__basic_data_types[idx] != DATA_TYPE_STRING) {
        // Type mismatch - return empty string
        __basic_data_ptr++;
        return "";
    }
    const char* str = (const char*)__basic_data[idx];
    __basic_data_ptr++;
    return str;
}

// RESTORE function
void basic_restore(int64_t index) {
    __basic_data_ptr = index;
}
```

---

### Phase 4: Code Generation for READ

**File**: `qbe_codegen_statements.cpp`

```cpp
void QBECodeGenerator::emitRead(const ReadStatement* stmt) {
    emitComment("READ " + joinStrings(stmt->variables, ", "));
    
    for (const auto& varName : stmt->variables) {
        VariableType varType = getVariableType(varName);
        std::string varRef = getVariableRef(varName);
        
        // Call appropriate runtime function
        std::string tempValue;
        if (varType == VariableType::INT) {
            tempValue = allocTemp("w");
            emit("    " + tempValue + " =w call $basic_read_int()\n");
        } else if (varType == VariableType::DOUBLE) {
            tempValue = allocTemp("d");
            emit("    " + tempValue + " =d call $basic_read_double()\n");
        } else if (varType == VariableType::STRING) {
            tempValue = allocTemp("l");
            emit("    " + tempValue + " =l call $basic_read_string()\n");
        }
        
        // Store to variable
        std::string qbeType = getQBEType(varType);
        emit("    " + varRef + " =" + qbeType + " copy " + tempValue + "\n");
    }
}
```

---

### Phase 5: Code Generation for RESTORE

**File**: `qbe_codegen_statements.cpp`

```cpp
void QBECodeGenerator::emitRestore(const RestoreStatement* stmt) {
    size_t targetIndex = 0;
    
    if (stmt->lineNumber > 0) {
        // RESTORE to line number
        auto it = m_lineRestorePoints.find(stmt->lineNumber);
        if (it != m_lineRestorePoints.end()) {
            targetIndex = it->second;
        }
        emitComment("RESTORE " + std::to_string(stmt->lineNumber));
    } else if (!stmt->label.empty()) {
        // RESTORE to label
        auto it = m_labelRestorePoints.find(stmt->label);
        if (it != m_labelRestorePoints.end()) {
            targetIndex = it->second;
        }
        emitComment("RESTORE " + stmt->label);
    } else {
        // RESTORE with no argument - reset to beginning
        emitComment("RESTORE");
    }
    
    // Call runtime function
    std::string indexTemp = allocTemp("l");
    emit("    " + indexTemp + " =l copy " + std::to_string(targetIndex) + "\n");
    emit("    call $basic_restore(l " + indexTemp + ")\n");
}
```

---

### Phase 6: Integration

**File**: `qbe_codegen_statements.cpp`

Add cases to `emitStatement()`:

```cpp
void QBECodeGenerator::emitStatement(const Statement* stmt) {
    switch (stmt->getType()) {
        // ... existing cases ...
        
        case ASTNodeType::STMT_READ:
            emitRead(static_cast<const ReadStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_RESTORE:
            emitRestore(static_cast<const RestoreStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_DATA:
            // DATA already processed by preprocessor - skip
            emitComment("DATA (preprocessed)");
            break;
    }
}
```

---

## Alternative: Simpler Array-Based Approach

**Pros**: Easier implementation, less runtime overhead  
**Cons**: Wastes space for mixed types

### Simplified Design

Store all values as 64-bit (widest type):

```qbe
data $__basic_data = align 8 {
    l 100,           # INT (stored as 64-bit)
    l 200,           # INT
    l $str_0,        # STRING pointer
    d d_3.14,        # DOUBLE
}

data $__basic_data_types = { 
    b 0, b 0, b 2, b 1 
}
```

READ functions interpret based on type tag.

---

## Testing Strategy

### Test 1: Basic DATA/READ

```basic
10 DATA 10, 20, 30
20 READ A%, B%, C%
30 PRINT A%, B%, C%
40 END
```

Expected: `10 20 30`

### Test 2: Mixed Types

```basic
10 DATA 42, 3.14, "Hello"
20 READ I%, D, S$
30 PRINT I%, D, S$
40 END
```

Expected: `42 3.14 Hello`

### Test 3: RESTORE

```basic
10 DATA 1, 2, 3
20 READ A%
30 PRINT A%
40 RESTORE
50 READ B%
60 PRINT B%
70 END
```

Expected: `1 1` (reads same value twice)

### Test 4: RESTORE to Label

```basic
10 DATA 100, 200
20 :DATASTART DATA 300, 400
30 READ A%
40 PRINT A%
50 RESTORE DATASTART
60 READ B%
70 PRINT B%
80 END
```

Expected: `100 300`

### Test 5: Type Coercion

```basic
10 DATA 42
20 READ D#
30 PRINT D#
40 END
```

Expected: `42.0` (INT coerced to DOUBLE)

---

## Implementation Checklist

### Phase 1: Infrastructure
- [ ] Re-enable DataPreprocessor in fbc_qbe.cpp
- [ ] Pass DataPreprocessorResult to QBECodeGenerator
- [ ] Add data storage members to QBECodeGenerator

### Phase 2: QBE Generation
- [ ] Implement emitDataSection()
- [ ] Emit data values array
- [ ] Emit type tags array
- [ ] Emit read pointer global

### Phase 3: Runtime
- [ ] Create runtime/basic_data.c
- [ ] Implement basic_read_int()
- [ ] Implement basic_read_double()
- [ ] Implement basic_read_string()
- [ ] Implement basic_restore()

### Phase 4: Code Generation
- [ ] Implement emitRead() in qbe_codegen_statements.cpp
- [ ] Implement emitRestore() in qbe_codegen_statements.cpp
- [ ] Add STMT_READ case to emitStatement()
- [ ] Add STMT_RESTORE case to emitStatement()
- [ ] Add STMT_DATA case (no-op) to emitStatement()

### Phase 5: Testing
- [ ] Test basic DATA/READ
- [ ] Test mixed types
- [ ] Test RESTORE
- [ ] Test RESTORE to label
- [ ] Test type coercion
- [ ] Test bounds checking (run out of data)

### Phase 6: Error Handling
- [ ] Handle READ past end of DATA
- [ ] Handle invalid RESTORE targets
- [ ] Handle type mismatches gracefully

---

## Performance Considerations

### Memory Overhead
- ~9 bytes per DATA value (8 bytes value + 1 byte type)
- Minimal for typical programs (<1KB for 100 values)

### Runtime Cost
- READ: 1 function call + 1 type check + 1 pointer increment
- RESTORE: 1 function call + 1 pointer assignment
- Very fast (comparable to array access)

### Optimization Opportunities
- Inline READ functions for known types
- Eliminate type checks when types match
- Use direct pointer arithmetic (no function calls)

---

## Files to Modify

| File | Changes |
|------|---------|
| `fbc_qbe.cpp` | Re-enable preprocessor, pass data to generator |
| `qbe_codegen_main.cpp` | Add data storage, emit data section |
| `qbe_codegen_statements.cpp` | Implement READ/RESTORE emission |
| `runtime/basic_data.c` | New file - runtime support |
| `build_fbc_qbe.sh` | Add basic_data.c to build |

**Estimated effort**: 4-6 hours

---

## Related Documentation

- [README.md](README.md) - Project overview
- [UserDefinedTypes.md](UserDefinedTypes.md) - Similar data layout concepts
- [arraysinfasterbasic.md](arraysinfasterbasic.md) - Array memory layout
