struct PhaseCopyCell { live:u8, peer:u8 }
fn choose_equal(phase:usize)->u8 {
 let formal_one=phase;
 let formal_two=formal_one;
 let selected=if(formal_two == usize(5)){u8(92)}else{u8(201)};
 let result_copy=selected;
 result_copy
}
fn main()->unit {
 let seed=usize(1);
 let start_one=seed;
 let start_two=start_one;
 let original=PhaseCopyCell{live:u8(166),peer:u8(28)};
 let result=loop(cursor=start_two){
  if(cursor < usize(8)){
   let actual_one=cursor;
   let actual_two=actual_one;
   let incoming=choose_equal(actual_two);
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
 let beside_final=original@peer;
 let result_copy=result;
 unit
}
