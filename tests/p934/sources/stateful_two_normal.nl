struct CounterPair { spare:u8, used:u8 }
fn set_value(field:ref<write,u8>,next:u8)->u8 {
    let old=replace(field,next);return old;
}
fn main()->unit {
    let first=CounterPair{spare:u8(11),used:u8(57)};
    let second=CounterPair{used:u8(39),spare:u8(226)};
    if(usize(2)<usize(9)) {
        let old=loan_write(first@used){|w|set_value(w,u8(24))};old;unit
    } else {
        let old=loan_write(first@used){|w|set_value(w,u8(81))};old;unit
    };
    second;unit
}
