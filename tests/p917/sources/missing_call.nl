struct QuotaWindow { spare:u8, budget:u8 }
fn exchange(slot:ref<write,u8>,proposed:u8)->u8 {
    let original=replace(slot,proposed);
    let saved=original;
    return saved;
}
fn main()->unit {
    let live=QuotaWindow{budget:u8(17),spare:u8(203)};
    let frame=QuotaWindow{spare:u8(61),budget:u8(87)};
    if (usize(3) < usize(8)) {
        loan_write(live@budget){|r|u8(17)}
    } else {
        loan_write(live@budget){|r|exchange(r,u8(122))}
    };
    let current=live@budget;
    current;unit
}
