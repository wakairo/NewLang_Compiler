struct Pair{right:u8,left:u8}
fn forward_value(arg:u8)->u8{arg}
fn main()->unit{
  let producer=Pair{right:u8(1),left:u8(80)};
  let consumer=Pair{right:u8(4),left:u8(9)};
  let prior_source=loan_write(producer@left){|source_ref|replace(source_ref,u8(123))};
  let carry=forward_value(prior_source);
  let prior_target=loan_write(consumer@right){|target_ref|replace(target_ref,carry)};
  let source_after=producer@left;
  let target_after=consumer@right;
  prior_target;source_after;target_after;unit
}
