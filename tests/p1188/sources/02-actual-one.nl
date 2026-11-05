struct PhaseCopyCell { live:u8, peer:u8 }
fn select_phase(phase:usize)->u8 {
 if(phase < usize(3)){u8(87)}else{u8(202)}
}
fn main()->unit {
 let original=PhaseCopyCell{live:u8(158),peer:u8(34)};
 let result=loop(cursor=usize(1)){
  if(cursor < usize(8)){
   let actual=cursor;
   let incoming=select_phase(actual);
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
