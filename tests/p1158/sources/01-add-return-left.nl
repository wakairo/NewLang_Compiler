struct LatchPair { latch:u8, stamp:u8 }
fn main()->unit {
 let original=LatchPair{latch:u8(83),stamp:u8(151)};
 let result=loop(cursor=usize(2)){
  if(cursor < usize(14)){continue(cursor + usize(5));}else{break cursor;}
 };
 let copied=result;
 let decision=copied < usize(18);
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
