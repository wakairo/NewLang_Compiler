struct Envelope { amount: u8, seal: u8 }
fn maybe_patch(write_to: ref<write,u8>, incoming: u8) -> unit {
    if (usize(44) == usize(45)) {
        let pre = replace(write_to, incoming);
        pre;
        unit
    } else {
        return unit;
    };
    unit
}
fn main() -> unit {
    let original = Envelope { seal: u8(161), amount: u8(48) };
    let observer = Envelope { amount: u8(216), seal: u8(81) };
    let before = original@seal;
    loan_write(original@amount) { |borrow| maybe_patch(borrow, u8(238)) };
    let after = original@seal;
    let receiver_old = loan_write(observer@amount) { |borrow| replace(borrow, u8(35)) };
    unit
}
