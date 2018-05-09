contract C {
	struct S { bool f; }
	S s;
    function f() internal view returns (S storage c) {
        while(false) {
            c = s; // warning expected
        }
    }
}
// ----
// Warning: (81-92): uninitialized storage pointer may be returned
