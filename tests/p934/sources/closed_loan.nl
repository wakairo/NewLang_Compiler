struct ClosedLease { control:u8, tag:u8 }
fn main()->unit {
    let first=ClosedLease{tag:u8(231),control:u8(29)};
    let second=ClosedLease{control:u8(93),tag:u8(148)};
    loan_write(first@control){|w|unit};
    if(usize(2)<usize(9)){unit}else{unit};
    unit
}
