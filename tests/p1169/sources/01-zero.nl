struct CounterNode { head:u8, tag:u8 }
fn main()->unit {
 let original=CounterNode{head:u8(45),tag:u8(173)};
 let result=loop(cursor=usize(9)){
  if(cursor < usize(9)){
   let old=loan_write(original@head){|writer|replace(writer,u8(121))};
   let old_copy=old;
   old_copy;
   let during=original@head;
   let beside=original@tag;
   during;
   beside;
   continue(cursor + usize(3));
  }else{break cursor;}
 };
 let final=original@head;
 let peer=original@tag;
 let copied=result;
 unit
}
