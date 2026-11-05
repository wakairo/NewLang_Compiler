struct PhaseCopyCell { live:u8, peer:u8 }
fn select_phase(phase:usize)->u8 {
 if(phase < usize(2)){u8(53)}else{u8(149)}
}
fn main()->unit {
 let original=PhaseCopyCell{live:u8(26),peer:u8(211)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(5)){
   let actual=cursor + usize(0);
   let incoming=select_phase(actual);
   let old=loan_write(original@live){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@live;
   let beside=original@peer;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@live;
 let beside_final=original@peer;
 let result_copy=result;
 unit
}
