contract C {
  function f() internal returns (uint, uint, uint, uint) {
    (uint a, uint b,,) = f();
    a; b;
  }
  function g() internal returns (bytes memory, string storage) {
    (bytes memory a, string storage b) = g();
    a; b;
  }
} 
