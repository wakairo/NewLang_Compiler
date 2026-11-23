fn main() -> unit {
    let rows = usize(23);
    let stride = usize(17);
    let area = rows * stride;
    let scaled = usize(29) * stride;
    let padded = rows * usize(11);
    let constant = usize(13) * usize(19);
    let empty = usize(0) * stride;
    let unchanged = rows * usize(1);
    let limit = usize(6148914691236517205);
    let boundary = limit * usize(3);
    let empty_max = usize(0) * usize(18446744073709551615);
    let verify = area == usize(391);
    padded;
    unit
}
