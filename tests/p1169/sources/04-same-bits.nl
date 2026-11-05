struct ModeNode { peer:u8, mode:u8 }
fn main()->unit {
 let original=ModeNode{peer:u8(182),mode:u8(54)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(3)){
   let old=loan_write(original@mode){|writer|replace(writer,u8(54))};
   let old_copy=old;
   old_copy;
   let during=original@mode;
   let beside=original@peer;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@mode;
 let peer=original@peer;
 let copied=result;
 unit
}
