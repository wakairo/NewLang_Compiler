struct ProbeCell { guarded:u8, kept:u8 }
fn choose_probe(phase:usize)->u8 {
 let sample=phase;
 if(sample == usize(1)){u8(151)}else{return u8(23);};
 u8(109)
}
fn main()->unit {
 let seed=usize(0);
 let start=seed;
 let original=ProbeCell{guarded:u8(73),kept:u8(219)};
 let result=loop(cursor=start){
  if(cursor < usize(4)){
   let actual=cursor;
   let incoming=choose_probe(actual);
   let old=loan_write(original@guarded){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@guarded;
   let beside=original@kept;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@guarded;
 let beside_final=original@kept;
 let result_copy=result;
 unit
}
