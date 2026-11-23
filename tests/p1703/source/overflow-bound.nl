fn wire_size(payload: usize) -> usize {
    if (payload < usize(18446744073709551615)) {
        let frame = payload + usize(2);
        frame
    } else {
        payload
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(18446744073709551614) } else { usize(18446744073709551615) };
    let size = wire_size(input);
    let queue = if (size < usize(18446744073709551615)) { usize(7) } else { usize(9) };
    let selected = queue;
    selected;
    unit
}
