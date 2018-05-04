contract C {
	struct S { bool f; }
	S s;
    function f(bool flag) internal view returns (S storage c) {
        flag ? c = s : c = s;
    }
    function f2(bool flag) internal view returns (S storage c) {
        flag ? c = s : (c = s);
    }
    function g(bool flag) internal view returns (S storage c) {
        flag ? (c = s).f : false; // expect warning
    }
    function h(bool flag) internal view returns (S storage c) {
        flag ? false : (c = s).f; // expect warning
    }
    function i(bool flag) internal view returns (S storage c) {
        flag ? (c = s).f : (c = s).f;
    }
}
// ----
// Warning: (293-304): uninitialized storage pointer may be returned
// Warning: (415-426): uninitialized storage pointer may be returned
