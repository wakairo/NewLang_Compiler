fn main()->unit{let x=u8(1);let p=loan_read(x){|r|ptr_from_ref(r)};let result=p < p;unit}
