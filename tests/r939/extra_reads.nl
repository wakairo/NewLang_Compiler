struct Meridian {
    dawn: u8,
    dusk: u8,
}

fn main() -> unit {
    let north = Meridian { dusk: u8(255), dawn: u8(0) };
    let south = Meridian { dawn: u8(73), dusk: u8(19) };
    let read_dawn = north@dawn;
    let read_dusk = north@dusk;
    let read_south_dawn = south@dawn;
    let read_south_dusk = south@dusk;
    if (usize(17) == usize(17)) { unit } else { unit };
    unit
}
