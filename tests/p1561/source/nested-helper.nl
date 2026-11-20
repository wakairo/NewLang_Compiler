fn block_slices(bytes: usize) -> usize {
    bytes / usize(8)
}

fn block_phase(slices: usize) -> usize {
    slices % usize(7)
}

fn nested_stage(bytes: usize) -> usize {
    let slices = block_slices(bytes);
    let phase = block_phase(slices);
    phase
}

fn main() -> unit {
    let phase = nested_stage(usize(157));
    unit
}
