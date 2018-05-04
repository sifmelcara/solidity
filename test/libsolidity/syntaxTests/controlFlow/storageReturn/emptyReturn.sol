contract C {
    struct S { bool f; }
    S s;
    function f() internal pure {}
    function g() internal pure returns (S storage) { return; }
    function h() internal view returns (S storage c, S storage) { c = s; return; }
    function i() internal view returns (S storage, S storage d) { d = s; return; }
    function j() internal view returns (S storage c, S storage d) { c = s; d = s; return; }
    function k() internal pure returns (S storage, S storage) { return; }
    function l() internal view returns (S storage, S storage) { return (s,s); }
}
// ----
// Warning: (121-122): uninitialized storage pointer may be returned
// Warning: (197-198): uninitialized storage pointer may be returned
// Warning: (267-268): uninitialized storage pointer may be returned
// Warning: (442-443): uninitialized storage pointer may be returned
// Warning: (453-454): uninitialized storage pointer may be returned
