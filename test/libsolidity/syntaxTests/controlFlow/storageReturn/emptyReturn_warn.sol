contract C {
    struct S { bool f; }
    S s;
    function f() internal pure returns (S storage) { return; }
    function g() internal view returns (S storage c, S storage) { c = s; return; }
    function h() internal view returns (S storage, S storage d) { d = s; return; }
    function i() internal pure returns (S storage, S storage) { return; }
    function j() internal view returns (S storage, S storage) { return (s,s); }
}
// ----
// Warning: (87-88): uninitialized storage pointer may be returned
// Warning: (163-164): uninitialized storage pointer may be returned
// Warning: (233-234): uninitialized storage pointer may be returned
// Warning: (316-317): uninitialized storage pointer may be returned
// Warning: (327-328): uninitialized storage pointer may be returned
