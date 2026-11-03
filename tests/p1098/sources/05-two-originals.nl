struct Packet { head: u8, tail: u8 }
fn main() -> unit {
    let untouched = Packet{head: u8(17), tail: u8(208)};
    let selected = Packet{tail: u8(129), head: u8(251)};
    let recovered = loop () {
        break loan_write(selected@head) { |write| replace(write, u8(36)) };
    };
    let displaced = loan_write(selected@tail) { |write| replace(write, recovered) };
    let first_head = untouched@head;
    let first_tail = untouched@tail;
    let second_head = selected@head;
    let second_tail = selected@tail;
    return unit;
}
