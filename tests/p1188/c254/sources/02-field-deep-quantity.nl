struct SnapshotCell { data:u8, audit:u8 }
fn choose_byte(tick:usize)->u8 {
 let visible=tick;
 let tested=visible;
 if(tested == usize(3)){u8(97)}else{u8(11)}
}
fn main()->unit {
 let original=SnapshotCell{audit:u8(11),data:u8(227)};
 let snap=original@audit;
 let first=snap;
 let held=first;
 let same_bits=u8(11);
 let seed=usize(1);
 let init_one=seed;
 let init_two=init_one;
 let result=loop(cursor=init_two){
  if(cursor < usize(6)){
   let capture_before=held;
   held;
   let actual_one=cursor;
   let actual_two=actual_one;
   cursor;
   let incoming=choose_byte(actual_two);
   let old=loan_write(original@audit){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@audit;
   let beside=original@data;
   during;
   beside;
   let capture_after=held;
   held;
   continue(cursor + usize(2));
  }else{break cursor;}
 };
 let final=original@audit;
 let beside_final=original@data;
 let after_saved=held;
 let result_copy=result;
 unit
}
