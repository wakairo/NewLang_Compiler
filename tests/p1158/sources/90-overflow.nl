struct LevelPair { peer:u8, level:u8 }
fn main()->unit {
 let original=LevelPair{peer:u8(37),level:u8(109)};
 let result=loop(cursor=usize(16)){
  if(cursor < usize(16)){continue(cursor + usize(18446744073709551615));}else{break cursor;}
 };
 let copied=result;
 let decision=copied < usize(16);
 let old=if(decision){
  return unit;
 }else{
  loan_write(original@level){|writer|replace(writer,u8(182))}
 };
 let updated=original@level;
 let sibling=original@peer;
 let old_copy=old;
 unit
}
