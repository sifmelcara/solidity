contract C {
        function f() public pure {
                C(bytes32(uint256(0x1234)));
        }
}
// ----
// TypeError: (64-91): Explicit type conversion not allowed from "bytes32" to "contract C".
