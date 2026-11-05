struct PhaseCell { live:u8, peer:u8 }
fn pick_phase(phase:usize)->u8 {
 if(phase < usize(4)){u8(65)}else{u8(181)}
}
fn main()->unit {
 let original=PhaseCell{live:u8(91),peer:u8(17)};
 let result=loop(cursor=usize(3)){
  if(cursor < usize(4)){
   let incoming=pick_phase(cursor);
   let old=loan_write(original@live){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@live;
   let beside=original@peer;
   during;
   beside;
   continue(cursor + usize(2));
  }else{break cursor;}
 };
 let final=original@live;
 let peer=original@peer;
 let copied=result;
 unit
}
