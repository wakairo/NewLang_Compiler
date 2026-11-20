fn shelf_count(items: usize) -> usize {
    let counted = items;
    let for_shelving = counted;
    let on_paper = for_shelving;
    let shelves = on_paper / usize(6);
    let staged = shelves;
    staged
}

fn main() -> unit {
    let count = usize(95);
    let full = shelf_count(count);
    let keep = full;
    let again = shelf_count(keep);
    let with_spare = full + usize(1);
    unit
}
