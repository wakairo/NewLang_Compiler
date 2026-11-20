fn main() -> unit {
    let trips = usize(74);
    let carried = trips;
    let full = carried / usize(8);
    let spill = carried % usize(8);
    let saved = full;
    let slots = saved + spill;
    let final_count = slots - usize(1);
    return unit;
}
