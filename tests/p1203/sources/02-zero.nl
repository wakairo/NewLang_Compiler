struct LedgerMark { ink:u8, keep:u8 }
fn shade_pick(position:usize)->u8 {
 if(position < usize(7)){u8(117)}else{u8(211)}
}
fn main()->unit {
 let original=LedgerMark{ink:u8(31),keep:u8(199)};
 let outcome=loop(tick=usize(10)){
  if(tick < usize(10)){
   let incoming=shade_pick(tick);
   let old=loan_write(original@ink){|write|replace(write,incoming)};
   let saved=old;
   saved;
   let during=original@ink;
   let sibling=original@keep;
   during;
   sibling;
   continue(tick + usize(2));
  }else{break usize(0);}
 };
 let final_ink=original@ink;
 let final_keep=original@keep;
 let answer_copy=outcome;
 unit
}
