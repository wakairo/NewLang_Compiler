struct PhaseCopyCell { live:u8, peer:u8 }
fn direct_pick(phase:usize)->u8 {
 if(phase < usize(1)){u8(41)}else{u8(142)}
}
fn main()->unit {
 let original=PhaseCopyCell{live:u8(8),peer:u8(233)};
 let result=loop(cursor=usize(0)){
  if(cursor < usize(3)){
   let incoming=direct_pick(cursor);
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
 let beside_final=original@peer;
 let result_copy=result;
 unit
}

fn audit_unused(first:usize, second:usize)->u8 { u8(9) }
