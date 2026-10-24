struct Heliograph { baseline: u8, zenith: u8 }
fn tilt(well: ref<write, u8>, offered: u8) -> unit {
    if (usize(13) < usize(41)) {
        let displaced = replace(well, offered);
        unit
    } else {
        return unit;
    };
    unit
}
fn main() -> unit {
    let signal = Heliograph { zenith: u8(214), baseline: u8(37) };
    let saved = signal@zenith;
    let neighbor = signal@baseline;
    loan_write(signal@zenith) { |lease|
        tilt(lease, u8(163))
    };
    let current_after = signal@zenith;
    let saved_after = saved;
    let neighbor_after = signal@baseline;
    let neighbor_snapshot_after = neighbor;
    unit
}
