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
    function f(bool flag) revertIfNoReturn() internal view returns(S storage) {
        if (flag) return s;
    }
    function g(bool flag) ifFlag(flag) internal view returns(S storage) {
        return s; // warning expected
    }

    function h(bool flag) ifFlag(flag) revertIfNoReturn() internal view returns(S storage) {
        return s; // warning expected
    }
    function i(bool flag) revertIfNoReturn() ifFlag(flag) internal view returns(S storage) {
        return s;
    }

}
// ----
// Warning: (363-364): uninitialized storage pointer may be returned
// Warning: (501-502): uninitialized storage pointer may be returned
