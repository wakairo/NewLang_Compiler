struct Pair { left:u8, right:u8, }
fn main()->unit {
    let control=Pair{right:u8(89),left:u8(212)};
    let old_left=loan_write(control@left){|slot|replace(slot,u8(6))};
    let written_left=control@left;
    let retained_right=control@right;
    old_left; written_left; retained_right; unit
}
