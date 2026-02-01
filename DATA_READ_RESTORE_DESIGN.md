# DATA/READ/RESTORE Implementation Design

## Overview

This document describes the implementation of BASIC's DATA/READ/RESTORE statements in the QBE code generator. The system provides a static data segment with runtime pointer management for sequential data reading.

---

## 1. Data Collection (Already Implemented)

The `DataPreprocessor` collects DATA statements before parsing:

**Input:**
```basic
DATA 10, 20, 30
DATA "Hello", "World"
START_LABEL:
DATA 100, 200
```

**Collected:**
- `values`: [10, 20, 30, "Hello", "World", 100, 200]
- `lineRestorePoints`: {line_number → index}
- `labelRestorePoints`: {"START_LABEL" → 5}

---

## 2. QBE IL Data Segment Emission

### 2.1 Global Data Labels

All DATA values are emitted as 64-bit (long/pointer) values for uniform access:

```qbe
# === DATA Segment ===

data $data_begins = { l 0 }  # Sentinel: marks start

# Actual DATA values (all 64-bit aligned)
data $data_0 = { l 10 }                # Integer promoted to long
data $data_1 = { l 20 }
data $data_2 = { l 30 }
data $data_3 = { l $str_0 }            # String pointer
data $data_4 = { l $str_1 }            # String pointer

# Label restore points
data $data_label_START_LABEL = { l 0 } # Points to $data_5 address

data $data_5 = { l 100 }
data $data_6 = { l 200 }

data $data_end = { l 0 }    # Sentinel: marks end
```

### 2.2 Global Data Pointer

A mutable global pointer tracks the current READ position:

```qbe
# === DATA Runtime State ===

data $__data_pointer = { l $data_0 }   # Initially points to first element
data $__data_start = { l $data_0 }     # Constant: first element
data $__data_end = { l $data_end }     # Constant: end sentinel
```

---

## 3. READ Statement Implementation

### 3.1 READ Algorithm

For each variable in the READ statement:
1. Load `$__data_pointer`
2. Check if pointer == `$__data_end` → ERROR: "Out of DATA"
3. Load value at pointer
4. Convert/assign to variable (handle type conversion)
5. Advance pointer by 8 bytes (sizeof(long))
6. Store updated `$__data_pointer`

### 3.2 QBE IL Example

**BASIC:**
```basic
DIM a AS INTEGER
DIM b AS STRING
READ a, b
```

**Generated QBE IL:**
```qbe
# READ a
%ptr0 =l loadl $__data_pointer
%end =l loadl $__data_end
%exhausted =w ceql %ptr0, %end
jnz %exhausted, @data_error, @read_ok_0

@read_ok_0
%val0 =l loadl %ptr0              # Load 64-bit value
%a =w copy %val0                  # Truncate to 32-bit int
%ptr1 =l add %ptr0, 8             # Advance 8 bytes
storel %ptr1, $__data_pointer

# READ b
%ptr2 =l loadl $__data_pointer
%exhausted2 =w ceql %ptr2, %end
jnz %exhausted2, @data_error, @read_ok_1

@read_ok_1
%val1 =l loadl %ptr2              # Load string pointer
# Store string pointer to variable b
storel %val1, $var_b_STRING
%ptr3 =l add %ptr2, 8
storel %ptr3, $__data_pointer
jmp @read_done

@data_error
call $fb_error_out_of_data()
call $exit(w 1)

@read_done
```

---

## 4. RESTORE Statement Implementation

### 4.1 RESTORE (no argument)

Reset to beginning of DATA:

```qbe
# RESTORE
%start =l loadl $__data_start
storel %start, $__data_pointer
```

### 4.2 RESTORE label

Restore to a labeled position:

**BASIC:**
```basic
RESTORE START_LABEL
```

**QBE IL:**
```qbe
# RESTORE START_LABEL
%label_pos =l copy $data_label_START_LABEL
storel %label_pos, $__data_pointer
```

### 4.3 RESTORE line_number

Restore to a line number position (requires line→index mapping):

**During codegen**, build a lookup table:
```cpp
std::map<int, size_t> lineRestorePoints;  // line → data index
```

**BASIC:**
```basic
RESTORE 1000
```

**QBE IL (runtime lookup):**
```qbe
# RESTORE 1000
# Option 1: Direct if known at compile time
%line_pos =l copy $data_line_1000
storel %line_pos, $__data_pointer

# Option 2: Runtime lookup (if dynamic)
call $fb_restore_to_line(w 1000)
```

---

## 5. Type Handling

### 5.1 All DATA as 64-bit Values

| BASIC Type | Storage          | READ Conversion       |
|------------|------------------|-----------------------|
| INTEGER    | `l 42`           | `%i =w copy %val`     |
| LONG       | `l 123456`       | `%lng =l copy %val`   |
| SINGLE     | `s s_3.14`       | `%sng =s exts %val`   |
| DOUBLE     | `d d_3.14159`    | `%dbl =d extsd %val`  |
| STRING     | `l $str_label`   | `%str =l copy %val`   |

**Note:** For SINGLE/DOUBLE, DATA values may need to be stored as actual float/double, not as longs. Alternative: store bit patterns and reinterpret.

### 5.2 Type Conversion on READ

If DATA contains `10` (integer) and READ tries to read into DOUBLE:
```qbe
%val =l loadl %ptr         # Load as long
%dbl =d sltof %val         # Convert signed long to double
```

If DATA contains string and READ tries to read into INTEGER:
→ **Runtime error** (type mismatch)

---

## 6. Runtime Error Handling

### 6.1 Out of DATA Error

```c
void fb_error_out_of_data() {
    fprintf(stderr, "Runtime Error: Out of DATA\n");
    // Could include line number context if available
}
```

### 6.2 Type Mismatch Error

```c
void fb_error_data_type_mismatch() {
    fprintf(stderr, "Runtime Error: DATA type mismatch\n");
}
```

### 6.3 Invalid RESTORE Target

```c
void fb_error_invalid_restore(int line) {
    fprintf(stderr, "Runtime Error: No DATA at line %d\n", line);
}
```

---

## 7. Implementation Steps

### Phase 1: Basic DATA Emission (✅ DONE)
- [x] Emit DATA values as global data segment
- [x] All values as 64-bit (long) where possible

### Phase 2: DATA Pointer and Sentinels
- [ ] Emit `$data_begins` sentinel
- [ ] Emit `$data_end` sentinel  
- [ ] Emit `$__data_pointer` global (initialized to first element)
- [ ] Emit `$__data_start` constant
- [ ] Emit `$__data_end` constant

### Phase 3: Label and Line Restore Points
- [ ] Emit `$data_label_XXX` for each labeled DATA position
- [ ] Emit `$data_line_NNNN` for each line-numbered DATA position
- [ ] Build compile-time mapping for RESTORE resolution

### Phase 4: READ Statement
- [ ] Add `STMT_READ` handler in `ast_emitter.cpp`
- [ ] Generate pointer load/check/advance sequence
- [ ] Handle multiple variables in single READ
- [ ] Type conversion logic
- [ ] Out-of-DATA error checking

### Phase 5: RESTORE Statement
- [ ] Add `STMT_RESTORE` handler in `ast_emitter.cpp`
- [ ] RESTORE (no arg) → reset to start
- [ ] RESTORE label → lookup label restore point
- [ ] RESTORE line → lookup line restore point

### Phase 6: Type Safety
- [ ] Store type metadata alongside DATA values (optional)
- [ ] Runtime type checking on READ
- [ ] Better error messages with context

---

## 8. Example Complete Program

**BASIC:**
```basic
DATA 10, 20, 30
DATA "Hello", "World"

START_AGAIN:
DATA 100, 200

DIM a AS INTEGER
DIM b AS INTEGER  
DIM s AS STRING

READ a, b          ' Read 10, 20
READ s             ' Read "Hello"
RESTORE
READ a             ' Read 10 again
RESTORE START_AGAIN
READ b             ' Read 100

PRINT a, b, s
END
```

**Generated QBE IL:**
```qbe
# === String Pool ===
data $str_0 = { b "Hello", b 0 }
data $str_1 = { b "World", b 0 }

# === DATA Segment ===
data $data_begins = { l 0 }

data $data_0 = { l 10 }
data $data_1 = { l 20 }
data $data_2 = { l $str_0 }
data $data_3 = { l $str_1 }

data $data_label_START_AGAIN = { l $data_4 }
data $data_4 = { l 100 }
data $data_5 = { l 200 }

data $data_end = { l 0 }

# === DATA Runtime State ===
data $__data_pointer = { l $data_0 }
data $__data_start = { l $data_0 }
data $__data_end_const = { l $data_end }

# === Main Program ===
export function w $main() {
@block_0
    # READ a, b
    call $__read_int(l $var_a_INT)
    call $__read_int(l $var_b_INT)
    
    # READ s
    call $__read_string(l $var_s_STRING)
    
    # RESTORE
    %start =l loadl $__data_start
    storel %start, $__data_pointer
    
    # READ a
    call $__read_int(l $var_a_INT)
    
    # RESTORE START_AGAIN
    %label_pos =l copy $data_label_START_AGAIN
    storel %label_pos, $__data_pointer
    
    # READ b
    call $__read_int(l $var_b_INT)
    
    # PRINT
    call $fb_print_int(w %var_a_INT)
    call $fb_print_int(w %var_b_INT)
    call $fb_print_string(l %var_s_STRING)
    
    ret 0
}
```

---

## 9. Runtime Helper Functions (CHOSEN APPROACH)

Use runtime helpers for type-safe DATA access:

```c
// runtime_c/data_runtime.c

// Global DATA runtime state (defined by codegen in QBE IL)
extern long* __data_pointer;      // Current read position
extern long* __data_start;        // First DATA element
extern long* __data_end_const;    // End sentinel

// Type tags array (parallel to data values)
// 0=int, 1=double, 2=string
extern int __data_types[];        

// Helper: get current data index
static inline size_t get_data_index() {
    return __data_pointer - __data_start;
}

// READ functions with type checking and conversion
int fb_read_int() {
    if (__data_pointer >= (long*)__data_end_const) {
        fb_error_out_of_data();
        exit(1);
    }
    size_t idx = get_data_index();
    int type = __data_types[idx];
    long value = *__data_pointer;
    __data_pointer++;
    
    if (type == 0) {  // int
        return (int)value;
    } else if (type == 1) {  // double stored as bit pattern
        // Reinterpret bits as double, then truncate to int
        union { long bits; double d; } u;
        u.bits = value;
        return (int)u.d;
    } else {
        fb_error_data_type_mismatch();
        exit(1);
    }
}

double fb_read_double() {
    if (__data_pointer >= (long*)__data_end_const) {
        fb_error_out_of_data();
        exit(1);
    }
    size_t idx = get_data_index();
    int type = __data_types[idx];
    long value = *__data_pointer;
    __data_pointer++;
    
    if (type == 0) {  // int
        return (double)value;
    } else if (type == 1) {  // double stored as bit pattern
        union { long bits; double d; } u;
        u.bits = value;
        return u.d;
    } else {
        fb_error_data_type_mismatch();
        exit(1);
    }
}

StringDescriptor* fb_read_string() {
    if (__data_pointer >= (long*)__data_end_const) {
        fb_error_out_of_data();
        exit(1);
    }
    size_t idx = get_data_index();
    int type = __data_types[idx];
    
    if (type != 2) {
        fb_error_data_type_mismatch();
        exit(1);
    }
    
    StringDescriptor* str = (StringDescriptor*)*__data_pointer;
    __data_pointer++;
    return str;
}

// RESTORE functions
void fb_restore() {
    __data_pointer = __data_start;
}

void fb_restore_to_label(long* label_pos) {
    __data_pointer = label_pos;
}

void fb_restore_to_line(long* line_pos) {
    __data_pointer = line_pos;
}
```

**Pros:**
- Simpler codegen (just call helper)
- Type safety with runtime checking
- Better error messages
- Easier to maintain

**Cons:**
- Runtime function call overhead (acceptable for DATA/READ)

---

## 10. Codegen Implementation

### READ Statement Codegen

**BASIC:**
```basic
READ a, b$, c#
```

**Generated QBE IL:**
```qbe
# READ a (integer)
%int_val =w call $fb_read_int()
storew %int_val, $var_a_INT

# READ b$ (string)
%str_val =l call $fb_read_string()
storel %str_val, $var_b_STRING

# READ c# (double)
%dbl_val =d call $fb_read_double()
stored %dbl_val, $var_c_DOUBLE
```

### RESTORE Statement Codegen

**BASIC:**
```basic
RESTORE              ' Reset to start
RESTORE MY_LABEL     ' Restore to label
RESTORE 1000         ' Restore to line
```

**Generated QBE IL:**
```qbe
# RESTORE
call $fb_restore()

# RESTORE MY_LABEL
%label_pos =l copy $data_label_MY_LABEL
call $fb_restore_to_label(l %label_pos)

# RESTORE 1000
%line_pos =l copy $data_line_1000
call $fb_restore_to_line(l %line_pos)
```

---

## 11. Open Questions

1. **Multi-dimensional arrays in DATA**: How to handle `DIM A(10)` with `READ A(I)`?
2. **DATA exhaustion behavior**: Error (chosen) or return sentinel value?
3. **String reference counting**: Should READ'd strings be retained/released?

---

## Summary

The DATA/READ/RESTORE system provides:
- Static data segment with all values as 64-bit elements
- Global pointer for sequential access
- Compile-time label/line resolution
- Runtime bounds checking
- Type conversion on READ
- Proper error handling

Next immediate action: Implement Phase 2 (sentinels and pointers).