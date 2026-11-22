fn reduce(sample: usize) -> usize {
    sample % usize(17)
}

fn route(input: usize, choose: bool) -> usize {
    let copied = input;
    let selection = if (choose) {
        let spare = copied / usize(4);
        reduce(copied / usize(4))
    } else {
        reduce(copied / usize(9))
    };
    selection
}

fn main() -> unit {
    let limit = usize(50);
    let yes = usize(44) < limit;
    let no = usize(113) < limit;
    let small = route(usize(44), yes);
    let large = route(usize(113), no);
    unit
}
