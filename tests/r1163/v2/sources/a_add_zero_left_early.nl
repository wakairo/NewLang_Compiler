struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(109), ember: u8(37) };
  let result = loop (cursor = usize(4)) {
    if (cursor < usize(4)) {
      continue(cursor + usize(1));
    } else {
      break usize(4);
    }
  };
  let result_copy = result;
  let old = if (result_copy == usize(4)) {
    return unit;
  } else {
    loan_write(vessel@tide) { |slot|
      replace(slot, u8(211))
    }
  };
  let old_copy = old;
  let current = vessel@tide;
  let sibling = vessel@ember;
  old_copy;
  current;
  sibling;
  unit
}
