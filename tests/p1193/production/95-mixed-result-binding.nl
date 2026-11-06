struct ExitCell { slot:u8, hold:u8 }
fn select_exit(phase:usize)->u8 {
 let observed=phase;
 let selected=if(observed < usize(2)){return u8(61);}else{u8(193)};
 let forwarded=selected;
 forwarded
}
fn main()->unit {
 let seed=usize(0);
 let start=seed;
 let original=ExitCell{slot:u8(85),hold:u8(221)};
 let result=loop(cursor=start){
  if(cursor < usize(4)){
   let actual=cursor;
   let incoming=select_exit(actual);
   let old=loan_write(original@slot){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@slot;
   let beside=original@hold;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@slot;
 let beside_final=original@hold;
 let result_copy=result;
 unit
}
