contract C {
    modifier revertIfNoReturn() {
        _;
        revert();
    }
    struct S { uint a; }
    S s;
    // currently this is expected to warn, but this should be changed in the future
    // (modifiers should be considered in the function control flow graph)
    function f(bool flag) revertIfNoReturn() internal view returns(S storage) {
        if (flag) return s;
    }
}
// ----
// Warning: (342-343): uninitialized storage pointer may be returned
