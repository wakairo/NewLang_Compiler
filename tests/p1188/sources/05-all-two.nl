struct PhaseCopyCell { live:u8, peer:u8 }
fn select_phase(phase:usize)->u8 {
 let formal_one=phase;
 let formal_two=formal_one;
 let selected=if(formal_two < usize(4)){u8(74)}else{u8(205)};
 let result_copy=selected;
 result_copy
}
fn main()->unit {
 let seed=usize(1);
 let start_one=seed;
 let start_two=start_one;
 let original=PhaseCopyCell{live:u8(144),peer:u8(19)};
 let result=loop(cursor=start_two){
  if(cursor < usize(7)){
   let actual_one=cursor;
   let actual_two=actual_one;
   let incoming=select_phase(actual_two);
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
