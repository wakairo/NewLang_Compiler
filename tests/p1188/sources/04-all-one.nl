struct PhaseCopyCell { live:u8, peer:u8 }
fn select_phase(phase:usize)->u8 {
 let observed=phase;
 if(observed < usize(2)){u8(33)}else{u8(177)}
}
fn main()->unit {
 let seed=usize(0);
 let start=seed;
 let original=PhaseCopyCell{live:u8(12),peer:u8(240)};
 let result=loop(cursor=start){
  if(cursor < usize(4)){
   let actual=cursor;
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
