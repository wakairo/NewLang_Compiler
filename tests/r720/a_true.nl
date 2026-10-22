struct CircuitTag { lane:u8, seal:u8 }
fn main()->unit {
 let root=CircuitTag{seal:u8(142),lane:u8(73)};
 let left=usize(4);let right=usize(10);
 let retired=if (left < right) {
   return unit;
 } else {
   loan_write(root@seal){|w|replace(w,u8(58))}
 };
 let active=root@seal;let other=root@lane;
 retired;active;other;unit
}
