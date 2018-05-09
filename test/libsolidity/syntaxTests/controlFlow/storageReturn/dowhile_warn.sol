contract C {
    struct S { bool f; }
    S s;
    function f() internal view returns (S storage c) {
        do {
            break;
            c = s;
        } while(false);
    }
    function g() internal view returns (S storage c) {
        do {
            if (s.f) {
                continue;
                c = s;
            }
            else {
            }
        } while(false);
    }
    function h() internal view returns (S storage c) {
        do {
            if (s.f) {
                break;
                continue;
            }
            else {
                c = s;
            }
        } while(false);
    }
}
// ----
// Warning: (87-98): uninitialized storage pointer may be returned
// Warning: (223-234): uninitialized storage pointer may be returned
// Warning: (440-451): uninitialized storage pointer may be returned
