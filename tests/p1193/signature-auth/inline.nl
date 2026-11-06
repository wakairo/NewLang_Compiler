struct ExitCell { slot:u8, hold:u8 }
fn select_exit(phase:usize)->u8 {
 let observed=phase;
 if(observed == usize(1)){return u8(61);}else{return u8(193);}
}
fn main()->unit {
 let seed=usize(0);
 let start=seed;
 let original=ExitCell{slot:u8(221),hold:u8(85)};
 let result=loop(cursor=start){
  if(cursor < usize(4)){
   let actual=cursor;
   let incoming=if(actual == usize(1)){u8(61)}else{u8(193)};
   let old=loan_write(original@hold){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@hold;
   let beside=original@slot;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@hold;
 let beside_final=original@slot;
 let result_copy=result;
 unit
}

fn audit_unused(first:usize, second:usize)->u8 { u8(9) }
