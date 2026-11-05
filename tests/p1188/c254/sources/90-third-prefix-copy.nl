struct SnapshotCell { data:u8, audit:u8 }
fn choose_byte(tick:usize)->u8 {
 if(tick < usize(2)){u8(43)}else{u8(186)}
}
fn main()->unit {
 let original=SnapshotCell{audit:u8(198),data:u8(72)};
 let snap=original@data;
 let copy_one=snap;
 let copy_two=copy_one;
 let held=copy_two;
 let peer_snap=original@audit;
 let peer_held=peer_snap;
 let same_bits=u8(72);
 let result=loop(cursor=usize(0)){
  if(cursor < usize(4)){
   let capture_before=held;
   held;
   let peer_before=peer_held;
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
   let peer_after=peer_snap;
   peer_held;
   continue(cursor + usize(1));
  }else{break cursor;}
 };
 let final=original@data;
 let beside_final=original@audit;
 let after_saved=held;
 let after_peer=peer_held;
 let result_copy=result;
 unit
}
