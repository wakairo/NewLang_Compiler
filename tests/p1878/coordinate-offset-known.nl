fn main() -> unit {
    let n = usize(13);
    let coordinate = uintptr(n);
    let displacement = usize(4);
    let forward = coordinate + displacement;
    let backward = forward - displacement;
    let matches = backward == coordinate;
    unit
}
