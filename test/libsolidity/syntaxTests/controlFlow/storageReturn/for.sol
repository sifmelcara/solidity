contract C {
	struct S { bool f; }
	S s;
    function f1() internal view returns (S storage c) {
        for(c = s;;) {
        }
    }
    function f2() internal view returns (S storage c) {
        for(; (c = s).f;) {
        }
    }
    function f3() internal view returns (S storage c) {
        // expect warning
        for(;; c = s) {
        }
    }
    function f4() internal view returns (S storage c) {
        // expect warning
        for(;;) {
            c = s;
        }
    }
}
// ----
// Warning: (277-288): uninitialized storage pointer may be returned
// Warning: (399-410): uninitialized storage pointer may be returned
