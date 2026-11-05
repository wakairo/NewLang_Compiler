struct PhaseCell { live:u8, peer:u8 }
fn main()->unit {
 let original=PhaseCell{live:u8(26),peer:u8(211)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(5)){
   let incoming=if(cursor < usize(2)){u8(53)}else{usize(149)};
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
