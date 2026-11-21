fn preset(order: usize) -> usize {
    let fixed = usize(91);
    fixed
}

fn main() -> unit {
    let order = usize(91);
    let echoed = preset(order);
    let ending = echoed % usize(5);
    unit
}
