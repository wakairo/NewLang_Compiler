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
    let reading = usize(octet);
    let snapshot = reading;
    let current = snapshot;
    let cap_octet = u8(219);
    let cap = widen_slots(cap_octet);
    let cap_copy = cap;
    let mid_octet = u8(150);
    let mid = widen_slots(mid_octet);
    let mid_copy = mid;
    let exact = current == cap_copy;
    let exact_reverse = cap_copy == current;
    let below = current < mid_copy;
    let above = mid_copy < current;
    let remembered = above;
    return unit;
}
