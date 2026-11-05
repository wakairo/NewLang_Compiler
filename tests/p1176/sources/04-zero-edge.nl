struct PhaseCell { live:u8, peer:u8 }
fn main()->unit {
 let original=PhaseCell{live:u8(192),peer:u8(37)};
 let result=loop(cursor=usize(7)){
  if(cursor < usize(6)){
   let incoming=if(cursor < usize(8)){u8(103)}else{u8(224)};
   let old=loan_write(original@peer){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@peer;
   let beside=original@live;
   during;
   beside;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@peer;
 let peer=original@live;
 let copied=result;
 unit
}
