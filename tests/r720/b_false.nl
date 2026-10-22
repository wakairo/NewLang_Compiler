struct SwitchUnit { latch:u8, flag:u8 }
fn main()->unit {
 let anchor=SwitchUnit{flag:u8(29),latch:u8(201)};
 let test=usize(18);let pivot=usize(12);
 let previous=if (test < pivot) {
   loan_write(anchor@latch){|slot|replace(slot,u8(166))}
 } else {
   let retired=loan_write(anchor@latch){|slot|replace(slot,u8(93))};
   retired;
   return unit;
 };
 let actual=anchor@latch;let peer=anchor@flag;
 previous;actual;peer;unit
}
