struct GatePair { tag:u8, gate:u8 }
fn main()->unit {
 let original=GatePair{tag:u8(174),gate:u8(61)};
 let result=loop(cursor=usize(27)){
  if(cursor < usize(9)){break cursor;}else{continue(cursor - usize(7));}
 };
 let copied=result;
 let decision=copied == usize(8);
 let old=if(decision){
  loan_write(original@gate){|writer|replace(writer,u8(211))}
 }else{
  return unit;
 };
 let updated=original@gate;
 let sibling=original@tag;
 let old_copy=old;
 unit
}
