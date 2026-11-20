fn buckets_for(records: usize) -> usize {
    let quotient = records / usize(5);
    let first_copy = quotient;
    let second_copy = first_copy;
    second_copy
}

fn main() -> unit {
    let coarse = buckets_for(usize(137));
    let fine = buckets_for(coarse);
    let kept_coarse = coarse;
    let with_reserve = fine + usize(4);
    unit
}
