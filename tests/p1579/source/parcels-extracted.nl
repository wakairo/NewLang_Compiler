fn packed(parcels: usize) -> usize {
    let stamped = parcels;
    let racks = stamped / usize(7);
    let transferred = racks;
    let slots = transferred % usize(9);
    let saved = slots;
    saved
}

fn main() -> unit {
    let parcels = usize(271);
    let slots = packed(parcels);
    let saved = slots;
    let observed = saved + usize(4);
    unit
}
