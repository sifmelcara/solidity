contract C {
    struct S { bool f; }
    S s;
    function g() internal view returns (S storage, uint) {
        return (s,2);
    }
    function f() internal view returns (S storage c) {
        uint a;
        (c, a) = g();
    }
}
// ----
