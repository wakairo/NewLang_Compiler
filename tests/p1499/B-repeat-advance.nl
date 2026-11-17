fn advance(occupied: usize) -> usize {
    if (occupied < usize(16)) {
        let next = occupied + usize(1);
        next
    } else {
        occupied
    }
}

fn main() -> usize {
    let next = advance(usize(11));
    let saved = next;
    let end = advance(saved);
    let result = end;
    result
}
