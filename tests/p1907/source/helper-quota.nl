fn read_slots(traffic: usize) -> u8 {
    if (traffic < usize(73)) { u8(37) } else { u8(219) }
}

fn widen_slots(octet: u8) -> usize {
    let widened = usize(octet);
    widened
}

fn main() -> unit {
    let load = usize(73);
    let octet = read_slots(load);
    let slots = widen_slots(octet);
    let exact = slots == usize(219);
    let below = slots < usize(150);
    let above = usize(150) < slots;
    let remembered = exact;
    return unit;
}
