fn unfinished_page(written: usize) -> usize {
    let occupancy = written % usize(16);
    let saved_occupancy = occupancy;
    saved_occupancy
}

fn main() -> unit {
    let written = usize(64);
    let leftover = unfinished_page(written);
    let remembered = leftover;
    let with_next_page = remembered + usize(16);
    unit
}
