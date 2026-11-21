fn main() -> unit {
    let pallets = usize(78);
    let ready = pallets;
    let selected = if (ready < usize(80)) {
        ready / usize(6)
    } else {
        ready % usize(65)
    };
    let saved = selected;
    let receipt = saved % usize(5);
    unit
}
