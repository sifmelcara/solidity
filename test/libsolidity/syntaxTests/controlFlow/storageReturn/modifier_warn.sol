contract C {
    modifier revertIfNoReturn() {
        _;
        revert();
    }
    modifier ifFlag(bool flag) {
        if (flag)
            _;
    }
    struct S { uint a; }
    S s;
    function f(bool flag) ifFlag(flag) internal view returns(S storage) {
        return s;
    }

    function g(bool flag) ifFlag(flag) revertIfNoReturn() internal view returns(S storage) {
        return s;
    }
}
// ----
// Warning: (249-250): uninitialized storage pointer may be returned
// Warning: (367-368): uninitialized storage pointer may be returned
