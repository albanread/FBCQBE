# Simple GOSUB/RETURN Design

## The Insight
GOSUB/RETURN is just:
- Jump to label (saving return address)
- Jump back to saved address

In assembly:
- ARM64: `bl label` / `ret`
- x86-64: `call label` / `ret`

## QBE Approach

Since QBE doesn't support indirect jumps, we need to use QBE's `call`/`ret` mechanism.

### Current: Everything in one QBE function
```qbe
function w $main() {
    @block_0:
        ...
    @block_9:
        # GOSUB - push block ID, jump
        storew 11, $return_stack[sp]
        jmp @block_27
    @block_11:
        # continues here after return
        ...
    @block_27:  # Subroutine
        ...
        # RETURN - pop and linear search(!)
        %id = loadw $return_stack[sp]
        compare id to 0, 1, 2, 3... 50 times
}
```

### Proposed: Use QBE call/ret
```qbe
function w $main() {
    @block_0:
        ...
    @block_9:
        # GOSUB - just call!
        call $Label_PrimeCheck()
    @block_11:
        # continues here automatically after return
        ...
}

function $Label_PrimeCheck() {
    # Access shared globals
    %q =l loadl $var_testq
    ...
    # RETURN - just ret!
    ret 0
}
```

## Implementation

1. Make all BASIC variables QBE globals (`data` declarations)
2. Split program at GOSUB target labels into separate QBE functions
3. GOSUB → `call $Label()`
4. RETURN → `ret 0`

Result:
- GOSUB: 1 instruction (call)
- RETURN: 1 instruction (ret)
- No custom stack, no linear search!
