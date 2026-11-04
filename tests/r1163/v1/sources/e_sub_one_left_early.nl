struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(149), ember: u8(83) };
  let result = loop (cursor = usize(19)) {
    if (usize(18) < cursor) {
      let step = cursor - usize(1);
      continue(step);
    } else {
      break cursor;
    }
  };
  let result_copy = result;
  let old = if (result_copy == usize(18)) {
    return unit;
  } else {
    loan_write(vessel@ember) { |slot|
      replace(slot, u8(227))
    }
  };
  let old_copy = old;
  let current = vessel@ember;
  let sibling = vessel@tide;
  old_copy;
  current;
  sibling;
  unit
}
