struct Pair { left:u8, right:u8 }
fn update_state(field:ref<write,u8>,next:u8)->u8{replace(field,next)}
fn main()->unit {
 let a=Pair{left:u8(7),right:u8(1)};
 let b=Pair{left:u8(9),right:u8(2)};
 let old_a=loan_write(a@left){|w|update_state(w,u8(11))};
 let old_b=loan_write(b@left){|w|update_state(w,u8(13))};
 let after_a=a@left;let marker_a=a@right;
 let after_b=b@left;let marker_b=b@right;
 old_a;old_b;after_a;marker_a;after_b;marker_b;unit
}
