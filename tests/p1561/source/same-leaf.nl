fn rows_for(pieces: usize) -> usize {
    let rows = pieces / usize(4);
    let saved_rows = rows;
    saved_rows
}

fn main() -> unit {
    let pieces = usize(84);
    let rows = rows_for(pieces);
    let carried = rows;
    let racks = rows_for(carried);
    let with_spares = rows + usize(3);
    unit
}
