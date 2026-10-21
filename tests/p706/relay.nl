struct Channel { idle:u8,active:u8 }
fn exchange(out:ref<write,u8>,next:u8)->u8{let old=replace(out,next);old}
fn main()->unit {
 let a=Channel{active:u8(80),idle:u8(39)};
 let b=Channel{idle:u8(160),active:u8(210)};
 let first=loan_write(a@active){|w|exchange(w,u8(0))};
 let second=loan_write(b@idle){|w|exchange(w,first)};
 let final=loan_write(a@idle){|w|replace(w,second)};
 let a0=a@idle;let a1=a@active;let b0=b@idle;let b1=b@active;
 first;second;final;a0;a1;b0;b1;unit
}
