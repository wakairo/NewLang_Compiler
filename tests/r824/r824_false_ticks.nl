struct Timer { ticks: u8, mode: u8 }
fn set_tick(target: ref<write,u8>, update: u8) -> u8 {
    replace(target, update)
}
fn main() -> unit {
    let clock = Timer { mode: u8(74), ticks: u8(28) };
    let sibling_before = clock@mode;
    if (false) {
        return unit;
    } else {
        let prior = loan_write(clock@ticks) { |write_tick| set_tick(write_tick, u8(16)) };
        let changed = clock@ticks;
        let sibling_during = clock@mode;
        prior;
        changed;
        sibling_during;
        unit
    };
    let ticks_after = clock@ticks;
    let mode_after = clock@mode;
    sibling_before;
    ticks_after;
    mode_after;
    unit
}
