contract C {
    function f() public pure returns(bytes4) {
        return bytes4(0x00001234);
    }
}
// ----
// TypeError: (75-93): Explicit type conversion not allowed from "int_const 4660" to "bytes4".
