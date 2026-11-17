fn next_bounded(count: usize) -> usize {
    if (count < usize(18446744073709551615)) {
        let next = count + usize(1);
        next
    } else {
        count
    }
}

fn main() -> usize {
    let next = next_bounded(usize(18446744073709551615));
    let retained = next;
    retained
}
