fn complete_pages(pending: usize) -> usize {
    let completed = pending / usize(4);
    let saved_completed = completed;
    return saved_completed;
}

fn main() -> unit {
    let pages = complete_pages(usize(31));
    let extra_pages = pages - usize(2);
    unit
}
