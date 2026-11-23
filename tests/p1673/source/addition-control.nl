fn main() -> unit {
    let rows = usize(23);
    let stride = usize(17);
    let product = rows + stride;
    let difference = product - usize(7);
    let matches = difference == usize(33);
    let less = rows < product;
    matches;
    less;
    unit
}
