struct TransitNode { marker:u8, level:u8 }
fn main()->unit {
 let original=TransitNode{marker:u8(236),level:u8(31)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(7)){
   let old=loan_write(original@level){|writer|replace(writer,u8(79))};
   let alias=old;
   let old_copy=alias;
   old_copy;
   let during=original@level;
   let beside=original@marker;
   during;
   beside;
   continue(cursor + usize(3));
  }else{break cursor;}
 };
 let final=original@level;
 let peer=original@marker;
 let copied=result;
 unit
}
