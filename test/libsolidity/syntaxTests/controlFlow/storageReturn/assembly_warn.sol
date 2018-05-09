contract C {
	struct S { bool f; }
	S s;
    function f() internal pure returns (S storage) {
        assembly {
        }
    }
}
// ----
// Warning: (81-82): uninitialized storage pointer may be returned
