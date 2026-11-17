fn next_occupancy(occupied: usize) -> usize {
    let decoy = usize(11);
    if (decoy < usize(16)) {
        let next = occupied + usize(1);
        next
    } else {
        occupied
    }
}

fn main() -> usize {
    next_occupancy(usize(1))
}
