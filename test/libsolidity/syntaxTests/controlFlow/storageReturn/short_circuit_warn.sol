contract C {
	struct S { bool f; }
	S s;
    function f() internal view returns (S storage c) {
        false && (c = s).f; // expect warning
    }
    function g() internal view returns (S storage c) {
        true || (c = s).f; // expect warning
    }
    function h() internal view returns (S storage c) {
        // expect warning as well (although this is always fine)
        true && (false || (c = s).f);
    }
}
// ----
// Warning: (81-92): uninitialized storage pointer may be returned
// Warning: (188-199): uninitialized storage pointer may be returned
// Warning: (294-305): uninitialized storage pointer may be returned
