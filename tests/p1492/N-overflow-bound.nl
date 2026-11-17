fn next_bounded(count: usize) -> usize {
    if (count < usize(18446744073709551615)) {
        let next = count + usize(2);
        next
    } else {
        count
    }
}

fn main() -> usize {
    next_bounded(usize(0))
}
