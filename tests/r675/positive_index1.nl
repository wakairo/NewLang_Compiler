struct Quiver { glow: u8, depth: u8 }
fn main() -> unit {
    let arrow = Quiver { glow: u8(29), depth: u8(173) };
    let before = arrow@depth;
    let neighbor_before = arrow@glow;
    let old = loan_write(arrow@depth) { |w| replace(w, u8(61)) };
    let after = arrow@depth;
    let neighbor_after = arrow@glow;
    before;
    neighbor_before;
    old;
    after;
    neighbor_after;
    unit
}
