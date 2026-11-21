fn main() -> unit {
    let pallets = usize(78);
    let ready = pallets;
    let groups = ready / usize(6);
    let spare = ready % usize(65);
    let selected = if (ready < usize(80)) {
        groups
    } else {
        spare
    };
    let saved = selected;
    let receipt = saved % usize(5);
    unit
}
