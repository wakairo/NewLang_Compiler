struct PhaseCopyCell { live:u8, peer:u8 }
fn direct_alias(phase:usize)->u8 {
 let selected=if(phase < usize(4)){u8(62)}else{u8(163)};
 let result_copy=selected;
 result_copy
}
fn main()->unit {
 let original=PhaseCopyCell{live:u8(117),peer:u8(46)};
 let result=loop(cursor=usize(2)){
  if(cursor < usize(6)){
   let incoming=direct_alias(cursor);
   let old=loan_write(original@peer){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@peer;
   let beside=original@live;
   during;
   beside;
   continue(cursor + usize(2));
  }else{break cursor;}
 };
 let final=original@peer;
 let beside_final=original@live;
 let result_copy=result;
 unit
}
