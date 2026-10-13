fn echo(x:u8)->u8{x} fn main()->unit{let x=u8(7);let p=loan_read(x){|r|ptr_from_ref(r)};echo(p);unit}
