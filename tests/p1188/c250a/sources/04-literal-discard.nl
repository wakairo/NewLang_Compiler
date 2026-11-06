struct LatchNode { latch:u8, echo:u8 }
fn main()->unit {
 let original=LatchNode{latch:u8(112),echo:u8(39)};
 let result=loop(cursor=usize(1)){
  if(cursor < usize(10)){
   let old=loan_write(original@latch){|writer|replace(writer,u8(203))};
   let old_copy=old;
   old_copy;
   let during=original@latch;
   let beside=original@echo;
   during;
   beside;
   cursor;
   continue(cursor + usize(4));
  }else{break cursor;}
 };
 let final=original@latch;
 let peer=original@echo;
 let copied=result;
 unit
}
