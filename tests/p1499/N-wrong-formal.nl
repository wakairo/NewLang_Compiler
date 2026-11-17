fn advance(occupied: usize) -> usize {
    if (occupied < usize(16)) {
        let next = occupied + usize(1);
        next
    } else {
        occupied
    }
}

fn reserve(offset: usize, decoy: usize) -> usize {
    if (decoy < usize(32)) {
        let end = offset + usize(4);
        end
    } else {
        offset
    }
}

fn main() -> usize {
    let next = advance(usize(11));
    let saved = next;
    let end = reserve(saved, usize(0));
    let result = end;
    result
}
