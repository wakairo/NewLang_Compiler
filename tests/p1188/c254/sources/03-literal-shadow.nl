struct SnapshotCell { data:u8, audit:u8 }
fn choose_byte(tick:usize)->u8 {
 if(tick < usize(1)){u8(163)}else{u8(29)}
}
fn main()->unit {
 let original=SnapshotCell{audit:u8(244),data:u8(163)};
 let memo=u8(163);
 let first=memo;
 let held=first;
 let same_bits=u8(163);
 let result=loop(cursor=usize(0)){
  if(cursor < usize(3)){
   let capture_before=held;
   held;
   let memo=u8(197);
   let local_shadow=memo;
   cursor;
   let incoming=choose_byte(cursor);
   let old=loan_write(original@data){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@data;
   let beside=original@audit;
   during;
   beside;
   let capture_after=held;
   held;
   let after_shadow=memo;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@data;
 let beside_final=original@audit;
 let after_saved=held;
 let result_copy=result;
 unit
}
