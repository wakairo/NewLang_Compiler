fn read_slots(traffic: usize) -> u8 {
    if (traffic < usize(73)) { u8(37) } else { u8(219) }
}

fn main() -> unit {
    let load = usize(73);
    let octet = read_slots(load);
    let slots = usize(octet);
    let exact = slots == usize(219);
    let next = if (exact) { u8(23) } else { u8(41) };
    return unit;
}
