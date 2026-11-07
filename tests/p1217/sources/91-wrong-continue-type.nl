struct Meter { active:u8, reserve:u8 }
fn select_code(tick:usize)->u8 {
 let argument=tick;
 let ignored=if(argument < usize(3)){return u8(29);}else{u8(181)};
 ignored;
 return u8(101);
}
fn main()->unit {
 let seed=usize(0);
 let start=seed;
 let original=Meter{active:u8(52),reserve:u8(197)};
 let finish=loop(index=start){
  if(index < usize(5)){
   let actual=index;
   let next_value=select_code(actual);
   let previous=loan_write(original@active){|writer|replace(writer,next_value)};
   let previous_copy=previous;
   previous_copy;
   let observed=original@active;
   let sibling=original@reserve;
   observed;
   sibling;
   continue(u8(1));
  }else{break index;}
 };
 let after=original@active;
 let unchanged=original@reserve;
 let count=finish;
 unit
}
