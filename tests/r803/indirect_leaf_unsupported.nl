fn main() -> unit {
    let leaf = Option<u8>::None;
    let indirect_cells = Array<Option<u8>,4>{
        leaf,
        leaf,
        leaf,
        leaf
    };
    unit
}
