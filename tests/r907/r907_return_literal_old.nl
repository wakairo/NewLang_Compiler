struct TideCache { seal: u8, level: u8 }

fn keep_tide(target: ref<write,u8>, arrival: u8) -> u8 {
    let before = replace(target, arrival);
    let echo = before;
    return u8(153);
}

fn main() -> u8 {
    let meter = TideCache { level: u8(153), seal: u8(41) };
    let reserve = TideCache { seal: u8(87), level: u8(12) };
    if (usize(3) == usize(8)) {
        u8(5)
    } else {
        u8(6)
    };
    if (usize(19) == usize(41)) {
        loan_write(meter@level) { |focus| keep_tide(focus, u8(217)) }
    } else {
        loan_write(meter@level) { |other_focus| keep_tide(other_focus, u8(102)) }
    };
    let actual = meter@level;
    let sibling = meter@seal;
    let control = reserve@level;
    return actual;
}
