struct Gauge { left:u8, right:u8 }
fn main()->unit {
 let original=Gauge{left:u8(38),right:u8(227)};
 let initial=usize(0);
 let start=initial;
 let count=loop(phase=start){
  if(phase < usize(3)){
   let observed=original@left;
   let sibling=original@right;
   observed;
   sibling;
   continue(phase + usize(1));
  }else{break phase;}
 };
 let after=original@left;
 let sibling=original@right;
 let result=count;
 unit
}
