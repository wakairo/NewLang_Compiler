struct SnapshotCell { data:u8, audit:u8 }
fn choose_byte(tick:usize)->u8 {
 if(tick < usize(2)){u8(38)}else{u8(151)}
}
fn main()->unit {
 let original=SnapshotCell{audit:u8(84),data:u8(212)};
 let snap=original@audit;
 let same_bits=u8(84);
 let result=loop(cursor=usize(2)){
  if(cursor < usize(3)){
   let capture_before=snap;
   snap;
   cursor;
   let incoming=choose_byte(cursor);
   let old=loan_write(original@audit){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let during=original@audit;
   let beside=original@data;
   during;
   beside;
   let capture_after=snap;
   snap;
   continue(cursor + usize(3));
  }else{break cursor;}
 };
 let final=original@audit;
 let beside_final=original@data;
 let after_saved=snap;
 let result_copy=result;
 unit
}
