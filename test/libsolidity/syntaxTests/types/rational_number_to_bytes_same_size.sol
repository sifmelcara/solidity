contract C {
    function f() public pure returns(bytes1) {
        return bytes1(0xAB);
    }
    function g() public pure returns(bytes2) {
        return bytes2(0xABAB);
    }
    function h() public pure returns(bytes4) {
        return bytes4(0xABABABAB);
    }
    function i() public pure returns(bytes8) {
        return bytes8(0xABABABABABABABAB);
    }
    function j() public pure returns(bytes16) {
        return bytes16(0xABABABABABABABABABABABABABABABAB);
    }
    function k() public pure returns(bytes32) {
        return bytes32(0xABABABABABABABABABABABABABABABABABABABABABABABABABABABABABABABAB);
    }
}
