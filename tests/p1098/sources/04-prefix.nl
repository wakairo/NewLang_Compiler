struct Register { left: u8, right: u8 }
fn main() -> unit {
    let register = Register{left: u8(21), right: u8(176)};
    let prefix = loan_write(register@left) { |write| replace(write, u8(88)) };
    let recovered = loop () {
        let previous = loan_write(register@right) { |write| replace(write, u8(244)) };
        break previous;
    };
    let displaced = loan_write(register@left) { |write| replace(write, recovered) };
    let left = register@left;
    let right = register@right;
    return unit;
}
