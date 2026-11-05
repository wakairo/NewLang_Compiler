struct SnapshotCell { data:u8, audit:u8 }
fn choose_byte(tick:usize)->u8 {
 if(tick < usize(2)){u8(37)}else{u8(204)}
}
fn main()->unit {
 let original=SnapshotCell{audit:u8(58),data:u8(218)};
 let snap=original@audit;
 let held=snap;
 let same_bits=u8(58);
 let result=loop(cursor=usize(4)){
  if(cursor < usize(4)){
   let capture_before=held;
   held;
   cursor;
   let incoming=choose_byte(cursor);
   let old=loan_write(original@audit){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@audit;
   let beside=original@data;
   during;
   beside;
   let capture_after=held;
   held;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@audit;
 let beside_final=original@data;
 let after_saved=held;
 let result_copy=result;
 unit
}
