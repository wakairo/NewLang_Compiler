struct PhaseCell { live:u8, peer:u8 }
fn main()->unit {
 let original=PhaseCell{live:u8(158),peer:u8(34)};
 let result=loop(cursor=usize(1)){
  if(cursor < usize(8)){
   let incoming=if(cursor < usize(4)){u8(87)}else{u8(202)};
   let chosen=incoming;
   let forwarded=chosen;
   let old=loan_write(original@peer){|writer|replace(writer,forwarded)};
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
 let peer=original@live;
 let copied=result;
 unit
}
