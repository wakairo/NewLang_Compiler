struct LedgerMark { ink:u8, keep:u8 }
fn shade_pick(position:usize)->u8 {
 if(position == usize(5)){u8(117)}else{u8(211)}
}
fn main()->unit {
 let original=LedgerMark{ink:u8(31),keep:u8(199)};
 let outcome=loop(tick=usize(3)){
  if(tick < usize(10)){
   let incoming=shade_pick(tick);
   let old=loan_write(original@keep){|write|replace(write,incoming)};
   let saved=old;
   saved;
   let during=original@keep;
   let sibling=original@ink;
   during;
   sibling;
   continue(tick + usize(2));
  }else{break usize(18446744073709551615);}
 };
 let final_ink=original@keep;
 let final_keep=original@ink;
 let answer_copy=outcome;
 unit
}
