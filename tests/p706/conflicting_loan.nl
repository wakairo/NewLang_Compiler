struct Tune { a:u8,b:u8 }
fn update(field:ref<write,u8>,next:u8)->u8{replace(field,next)}
fn main()->unit{let x=Tune{a:u8(1),b:u8(2)};let old=loan_write(x@a){|w|let other=loan_exclusive_write(x@a){|e|update(w,u8(9))};other};old;unit}
