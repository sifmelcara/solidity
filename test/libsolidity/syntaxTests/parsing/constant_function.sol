contract C {
    function f() constant {}
}
// ----
// ParserError: (30-30): The state mutability modifier "constant" was removed in 0.5.0. Use "view" or "pure" instead.
