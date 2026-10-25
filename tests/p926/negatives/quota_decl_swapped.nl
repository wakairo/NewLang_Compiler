struct QuotaLatch { reserve:u8, level:u8 }
fn zexchange(w:ref<write,u8>,incoming:u8)->u8 {
    let previous=replace(w,incoming);
    return previous;
}
fn main()->unit {
    let live=QuotaLatch{reserve:u8(77),level:u8(149)};
    let idle=QuotaLatch{level:u8(28),reserve:u8(215)};
    if (usize(3)==usize(3)) {
        loan_write(live@level){|r|zexchange(r,u8(51))}
    } else {
        loan_write(live@level){|r|zexchange(r,u8(92))}
    };
    unit
}
