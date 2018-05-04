contract C {
	struct S { bool f; }
	S s;
    function f() internal view returns (S storage c) {
        (c = s).f && false;
    }
    function f2() internal view returns (S storage c) {
        false && (c = s).f; // expect warning
    }
    function g() internal view returns (S storage c) {
        (c = s).f || true;
    }
    function h() internal view returns (S storage c) {
        true || (c = s).f; // expect warning
    }
    function i() internal view returns (S storage c) {
        // expect warning as well (although this is always fine)
        true && (false || (c = s).f);
    }
}
// ----
// Warning: (171-182): uninitialized storage pointer may be returned
// Warning: (366-377): uninitialized storage pointer may be returned
// Warning: (472-483): uninitialized storage pointer may be returned
