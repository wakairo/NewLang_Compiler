fn scale_slots(flag: bool) -> usize {
    let load = if (flag) {
        usize(107)
    } else {
        usize(61)
    };
    if (flag) {
        load % usize(9)
    } else {
        load / usize(8)
    }
}

fn main() -> unit {
    let threshold = usize(80);
    let yes = usize(44) < threshold;
    let no = usize(117) < threshold;
    let first = scale_slots(yes);
    let second = scale_slots(no);
    unit
}
