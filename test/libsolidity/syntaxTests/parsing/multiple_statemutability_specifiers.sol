contract c1 {
    function f() payable payable {}
}
contract c2 {
    function f() view view {}
}
contract c3 {
    function f() pure pure {}
}
contract c4 {
    function f() pure view {}
}
contract c5 {
    function f() payable view {}
}
contract c6 {
    function f() pure payable {}
}
contract c7 {
    function f() pure constant {}
}
// ----
// ParserError: (39-39): State mutability already specified as "payable".
// ParserError: (88-88): State mutability already specified as "view".
// ParserError: (134-134): State mutability already specified as "pure".
// ParserError: (180-180): State mutability already specified as "pure".
// ParserError: (229-229): State mutability already specified as "payable".
// ParserError: (275-275): State mutability already specified as "pure".
// ParserError: (324-324): State mutability already specified as "pure".
