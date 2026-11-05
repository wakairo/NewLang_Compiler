struct BoundaryNode { flag:u8, side:u8 }
fn main()->unit {
 let original=BoundaryNode{flag:u8(17),side:u8(214)};
 let result=loop(cursor=usize(18446744073709551613)){
  if(cursor < usize(18446744073709551614)){
   let old=loan_write(original@flag){|writer|replace(writer,u8(88))};
   let old_copy=old;
   old_copy;
   let during=original@flag;
   let beside=original@side;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@flag;
 let peer=original@side;
 let copied=result;
 unit
}
