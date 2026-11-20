fn shifted_phase(position: usize) -> usize {
    let phase = position % usize(11);
    let saved_phase = phase;
    let shifted = saved_phase + usize(1);
    shifted
}

fn main() -> unit {
    let favorable = shifted_phase(usize(33));
    unit
}
