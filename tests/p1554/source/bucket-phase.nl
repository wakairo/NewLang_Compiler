fn bucket_phase(index: usize) -> usize {
    let phase = index % usize(11);
    let saved_phase = phase;
    return saved_phase;
}

fn main() -> unit {
    let first_phase = bucket_phase(usize(152));
    let reduced_again = bucket_phase(first_phase);
    let translated = reduced_again + usize(11);
    let translated_phase = bucket_phase(translated);
    let first_again = first_phase;
    let room = usize(11) - translated_phase;
    unit
}
