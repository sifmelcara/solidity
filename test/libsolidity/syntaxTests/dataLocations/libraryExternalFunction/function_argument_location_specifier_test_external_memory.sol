library test {
    function f(bytes memory) external;
}
// ----
// TypeError: (30-35): Location has to be calldata or storage for external function in library.
