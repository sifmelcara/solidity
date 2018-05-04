contract C {
	struct S { bool f; }
	S s;
    function f() internal pure returns (S storage) {
        // expect warning
        assembly {
        }
    }
    function f2() internal returns (S storage c) {
        assembly {
            sstore(c_slot, sload(s_slot))
        }
    }
    function f3(bool flag) internal returns (S storage c) {
        // control flow in assembly will not be analyzed for now,
        // so this will not issue a warning
        assembly {
            if flag {
                sstore(c_slot, sload(s_slot))
            }
        }
    }
    function f4() internal returns (S storage c) {
        // any reference from assembly will be sufficient for now,
        // so this will not issue a warning
        assembly {
            sstore(s_slot, sload(c_slot))
        }
    }
}
// ----
// Warning: (81-82): uninitialized storage pointer may be returned
