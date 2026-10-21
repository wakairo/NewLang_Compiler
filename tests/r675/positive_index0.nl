struct Fossil { seam: u8, quartz: u8 }
fn main() -> unit {
    let shard = Fossil { quartz: u8(37), seam: u8(91) };
    let before = shard@seam;
    let neighbor_before = shard@quartz;
    let old = loan_write(shard@seam) { |w| replace(w, u8(146)) };
    let after = shard@seam;
    let neighbor_after = shard@quartz;
    before;
    neighbor_before;
    old;
    after;
    neighbor_after;
    unit
}
