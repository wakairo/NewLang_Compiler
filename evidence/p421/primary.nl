struct Pair{left:u8,right:u8}
fn relay(x:u8)->u8{x}
fn main()->unit{
  let a=Pair{left:u8(7),right:u8(1)};
  let b=Pair{left:u8(9),right:u8(2)};
  let old_a=loan_write(a@left){|w|replace(w,u8(11))};
  let transported=relay(old_a);
  let old_b=loan_write(b@right){|w|replace(w,transported)};
  let after_a=a@left;
  let after_b=b@right;
  old_b;after_a;after_b;unit
}
