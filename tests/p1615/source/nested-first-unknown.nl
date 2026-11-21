fn remainder(seed: usize) -> usize {
    seed % usize(13)
}

fn select_pair(left: usize, right: usize) -> usize {
    let answer = if (left < usize(30)) {
        left
    } else {
        right
    };
    let copied = answer;
    copied;
    copied
}

fn first_of(first: usize, ignored: usize) -> usize {
    let returned = first;
    returned;
    returned
}

fn main() -> unit {
    let early = first_of(select_pair(usize(23), usize(68)), remainder(usize(30)));
    early;
    let late = first_of(select_pair(usize(43), usize(19)), remainder(usize(45)));
    late;
    let residue = late % usize(7);
    residue;
    unit
}
