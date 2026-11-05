struct TonePair { tone:u8, echo:u8 }
fn main()->unit {
 let original=TonePair{tone:u8(94),echo:u8(206)};
 let result=loop(cursor=usize(9)){
  if(cursor < usize(12)){continue(cursor + usize(4));}else{break cursor;}
 };
 let copied=result;
 let decision=copied == usize(13);
 let old=if(decision){
  loan_write(original@tone){|writer|replace(writer,u8(129))}
 }else{
  return unit;
 };
 let updated=original@tone;
 let sibling=original@echo;
 let old_copy=old;
 unit
}
