contract C {
    struct S { bool f; }
    S s;
    function f1() internal view returns (S storage c) {
        do {} while((c = s).f);
    }
    function f2() internal view returns (S storage c) {
        do { c = s; } while(false);
    }
    function f3() internal view returns (S storage c) {
        c = s;
        do {} while(false);
    }
    function f4() internal view returns (S storage c) {
        do {} while(false);
        c = s;
    }
    function f5() internal view returns (S storage c) {
        do {
            c = s;
            break;
        } while(false);
    }
    function f6() internal view returns (S storage c) {
        do {
            break;
            c = s; // expected warning
        } while(false);
    }
    function f7() internal view returns (S storage c) {
        do {
            if (s.f) {
                continue;
                c = s; // expected warning
            }
            else {
            }
        } while(false);
    }
    function f8() internal view returns (S storage c) {
        do {
            if (s.f) {
                continue;
                break;
            }
            else {
                c = s;
            }
        } while(false);
    }
    function f9() internal view returns (S storage c) {
        do {
            if (s.f) {
                break; // expected warning
                continue;
            }
            else {
                c = s;
            }
        } while(false);
    }
}
// ----
// Warning: (627-638): uninitialized storage pointer may be returned
// Warning: (784-795): uninitialized storage pointer may be returned
// Warning: (1263-1274): uninitialized storage pointer may be returned
