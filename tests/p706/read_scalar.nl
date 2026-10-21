fn update(field:ref<write,u8>,next:u8)->u8{replace(field,next)}
fn main()->unit{let x=u8(31);let old=loan_read(x){|r|update(r,u8(9))};old;unit}
