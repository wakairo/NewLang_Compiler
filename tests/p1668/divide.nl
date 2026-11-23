fn choose_volume(flag: bool) -> usize {
    let stock = if (flag) {
        usize(97)
    } else {
        usize(54)
    };
    if (flag) {
        stock / usize(7)
    } else {
        stock / usize(5)
    }
}

fn main() -> unit {
    let limit = usize(100);
    let first_flag = usize(64) < limit;
    let second_flag = usize(124) < limit;
    let first = choose_volume(first_flag);
    let second = choose_volume(second_flag);
    unit
}
