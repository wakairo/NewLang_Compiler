struct PhaseCell { live:u8, peer:u8 }
fn pick_phase(phase:usize)->u8 {
 if(phase < usize(2)){u8(26)}else{u8(26)}
}
fn main()->unit {
 let original=PhaseCell{live:u8(26),peer:u8(211)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(5)){
   let incoming=pick_phase(cursor);
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
 let peer=original@peer;
 let copied=result;
 unit
}
