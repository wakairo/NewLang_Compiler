struct FrameCounter { count:u8, pad:u8 }
fn main()->unit {
    let cold=FrameCounter{count:u8(33),pad:u8(160)};
    let warm=FrameCounter{pad:u8(41),count:u8(203)};
    if (usize(4)==usize(9)) {
        loan_write(warm@count){|r|adjust(r,u8(11))}
    } else {
        loan_write(warm@count){|r|adjust(r,u8(87))}
    };
    unit
}
fn adjust(target:ref<write,u8>,next:u8)->u8 {
    let prior=replace(target,next);
    let preserved=prior;
    return preserved;
}
