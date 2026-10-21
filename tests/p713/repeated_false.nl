struct Profile { low:u8, high:u8 }
fn main()->unit {
 let state=Profile{high:u8(72),low:u8(11)};
 let amount=usize(8);let cutoff=usize(5);
 let old=if (amount < cutoff) {
   let first=loan_write(state@high){|w|replace(w,u8(9))};
   let seen=state@high;
   let second=loan_write(state@high){|w|replace(w,first)};
   seen;second
 } else {
   let first=loan_write(state@high){|w|replace(w,u8(240))};
   let second=loan_write(state@high){|w|replace(w,u8(61))};
   first;second
 };
 let current=state@high;let sibling=state@low;
 old;current;sibling;unit
}
