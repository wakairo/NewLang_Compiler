struct LatchPair { latch:u8, stamp:u8 }
fn main()->unit {
 let original=LatchPair{latch:u8(83),stamp:u8(151)};
 let result=usize(3);
 let copied=result;
 let decision=copied < usize(9);
 let old=if(decision){
  return unit;
 }else{
  loan_write(original@latch){|writer|replace(writer,u8(204))}
 };
 let updated=original@latch;
 let sibling=original@stamp;
 let old_copy=old;
 unit
}
