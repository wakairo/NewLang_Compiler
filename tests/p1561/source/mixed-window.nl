fn lane_count(slices: usize) -> usize {
    slices / usize(3)
}

fn window_phase(slices: usize) -> usize {
    let phase = slices % usize(7);
    let saved_phase = phase;
    return saved_phase;
}

fn slice_count(bytes: usize) -> usize {
    bytes / usize(8)
}

fn main() -> unit {
    let slices = slice_count(usize(157));
    let carried_slices = slices;
    let phase = window_phase(carried_slices);
    let available_slices = slices - usize(3);
    let lanes = lane_count(available_slices);
    let phase_copy = phase;
    let phase_lanes = lane_count(phase_copy);
    let next_phase = window_phase(usize(26));
    let retained_phase = phase;
    let free_slots = usize(8) - retained_phase;
    unit
}
