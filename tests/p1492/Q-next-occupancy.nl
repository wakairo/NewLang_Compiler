fn next_occupancy(occupied: usize) -> usize {
    if (occupied < usize(16)) {
        let next = occupied + usize(1);
        next
    } else {
        occupied
    }
}

fn main() -> usize {
    let next = next_occupancy(usize(11));
    let saved = next;
    saved
}
