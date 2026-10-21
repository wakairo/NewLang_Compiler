fn invalid(field:ref<write,u8>,next:u8)->u8{let x=u8(4);let saved=loan_write(x){|inner|inner};replace(field,next)}
fn main()->unit{unit}
