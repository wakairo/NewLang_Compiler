fn transit(item:u8)->u8{item}
fn main()->unit{
    let zero=transit(u8(0));
    let full=transit(u8(255));
    let zero_again=transit(zero);
    let full_again=transit(full);
    zero_again;
    full_again;
    unit
}
