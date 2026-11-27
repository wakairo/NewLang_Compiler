fn main() -> unit {
    let n = usize(13);
    let coordinate = uintptr(n);
    let m = usize(19);
    let distinct = uintptr(m);
    let copy = coordinate;
    let same_eq = copy == coordinate;
    let same_ne = copy != coordinate;
    let distinct_eq = copy == distinct;
    let distinct_ne = copy != distinct;
    unit
}
