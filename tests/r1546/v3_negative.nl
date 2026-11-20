fn reject_unbounded_division(active_items: usize) -> usize {
    active_items / usize(0)
}

fn main() -> unit {
    let rejected = reject_unbounded_division(usize(73));
    let copied = rejected;
    let increased = copied + usize(1);
    unit
}
