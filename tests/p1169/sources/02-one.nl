struct BeaconNode { marker:u8, level:u8 }
fn main()->unit {
 let original=BeaconNode{marker:u8(67),level:u8(156)};
 let result=loop(cursor=usize(2)){
  if(cursor < usize(4)){
   let old=loan_write(original@level){|writer|replace(writer,u8(98))};
   let old_copy=old;
   old_copy;
   let during=original@level;
   let beside=original@marker;
   during;
   beside;
   continue(cursor + usize(5));
  }else{break cursor;}
 };
 let final=original@level;
 let peer=original@marker;
 let copied=result;
 unit
}
