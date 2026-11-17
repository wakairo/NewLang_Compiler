fn advance(occupied: usize) -> usize {
    if (occupied < usize(16)) {
        let next = occupied + usize(1);
        next
    } else {
        occupied
    }
}

fn reserve(offset: usize) -> usize {
    if (offset < usize(18446744073709551615)) {
        let end = offset + usize(2);
        end
    } else {
        offset
    }
}

fn main() -> usize {
    let next = advance(usize(11));
    let saved = next;
    let end = reserve(saved);
    let result = end;
    result
}
