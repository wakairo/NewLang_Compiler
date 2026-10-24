struct Meter { mark: u8, seal: u8 }
fn edit(current: ref<write,u8>, replacement: u8) -> unit {
    if (usize(73) == usize(73)) {
        let removed = replace(current, replacement);
        removed;
        unit
    } else {
        return unit;
    };
    unit
}
fn main() -> unit {
    let origin = Meter { seal: u8(109), mark: u8(67) };
    let receiver = Meter { mark: u8(211), seal: u8(29) };
    let sibling_before = origin@seal;
    loan_write(origin@mark) { |q| edit(q, u8(143)) };
    let sibling_after = origin@seal;
    let receiver_old = loan_write(receiver@mark) { |q| replace(q, u8(53)) };
    unit
}
