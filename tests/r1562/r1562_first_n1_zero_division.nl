fn main() -> unit {
    let load = usize(53);
    let first = sift_five(load);
    let saved = first;
    let second = pack_three(saved);
    let repeated = pack_three(first);
    let load_copy = load;
    let second_copy = second;
    unit
}

fn pack_three(parcel: usize) -> usize {
    let chunk = parcel / usize(3);
    let chunk_copy = chunk;
    return chunk_copy;
}

fn sift_five(parcel: usize) -> usize {
    let chunk = parcel / usize(0);
    chunk
}
