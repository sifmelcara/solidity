contract C {
	struct S { bool f; }
	S s;
    function f(bool flag) internal view returns (S storage c) {
        if (flag) c = s;
    }
    function g(bool flag) internal returns (S storage c) {
        if (flag) c = s;
        else
        {
            if (!flag) c = s;
            else s.f = true;
        }
    }
}
// ----
// Warning: (90-101): uninitialized storage pointer may be returned
// Warning: (180-191): uninitialized storage pointer may be returned
