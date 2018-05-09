contract C {
	struct S { bool f; }
	S s;
    function f(bool flag) internal view returns (S storage c) {
        flag ? (c = s).f : false; // expect warning
    }
    function g(bool flag) internal view returns (S storage c) {
        flag ? false : (c = s).f; // expect warning
    }
}
// ----
// Warning: (90-101): uninitialized storage pointer may be returned
// Warning: (212-223): uninitialized storage pointer may be returned
