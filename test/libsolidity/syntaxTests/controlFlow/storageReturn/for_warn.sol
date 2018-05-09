contract C {
	struct S { bool f; }
	S s;
    function f() internal view returns (S storage c) {
        for(;; c = s) {
        }
    }
    function g() internal view returns (S storage c) {
        for(;;) {
            c = s;
        }
    }
}
// ----
// Warning: (81-92): uninitialized storage pointer may be returned
// Warning: (176-187): uninitialized storage pointer may be returned
