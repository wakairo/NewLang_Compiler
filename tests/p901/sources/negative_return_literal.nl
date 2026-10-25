struct ScanPolicy { marker:u8, quota:u8 }
fn set_quota(access:ref<write,u8>,proposed:u8)->u8 {
    let old=replace(access,proposed);
    let carried=old;
    return u8(56);
}
fn main()->unit {
    let active=ScanPolicy{quota:u8(56),marker:u8(11)};
    let mirror=ScanPolicy{marker:u8(187),quota:u8(9)};
    if (usize(1) == usize(1)) {u8(4)} else {u8(5)};
    if (usize(2) < usize(7)) {
        loan_write(active@quota){|w|set_quota(w,u8(126))}
    } else {
        loan_write(active@quota){|w|set_quota(w,u8(209))}
    };
    let current=active@quota;
    let other=mirror@marker;
    current;other;unit
}
