fn wire_size(payload: usize) -> usize {
    if (payload < usize(497)) {
        let frame = payload + usize(16);
        frame
    } else {
        payload
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(496) } else { usize(497) };
    let size = wire_size(input);
    let queue = if (size < usize(504)) { usize(7) } else { usize(9) };
    let selected = queue;
    selected;
    unit
}
