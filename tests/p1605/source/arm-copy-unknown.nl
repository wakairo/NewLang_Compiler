fn route(inventory: usize) -> usize {
    let input = inventory;
    let merged = if (input < usize(20)) { input } else { input };
    let copy = merged;
    let choice = if (input < usize(10)) {
        let exact = input / usize(5);
        let groups = copy / usize(5);
        let saved = groups;
        saved
    } else {
        let leftover = copy % usize(5);
        let saved = leftover;
        saved
    };
    choice
}

fn main() -> unit {
    let zero = route(usize(0));
    let near = route(usize(9));
    let far = route(usize(26));
    unit
}
