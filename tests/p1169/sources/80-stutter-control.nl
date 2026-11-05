struct LatchNode { latch:u8, echo:u8 }
fn main()->unit {
 let original=LatchNode{latch:u8(112),echo:u8(39)};
 let result=loop(cursor=usize(1)){
  if(cursor < usize(10)){
   continue(cursor + usize(4));
  }else{break cursor;}
 };
 let final=original@latch;
 let peer=original@echo;
 let copied=result;
 unit
}
