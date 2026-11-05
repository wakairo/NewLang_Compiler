struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(131), ember: u8(71) };
  let result = loop (cursor = usize(8)) {
    if (cursor < usize(9)) {
      break cursor;
    } else {
      continue(cursor - usize(1));
    }
  };
  let result_copy = result;
  let old = if (result_copy == usize(8)) {
    loan_write(vessel@tide) { |slot|
      replace(slot, u8(223))
    }
  } else {
    return unit;
  };
  let old_copy = old;
  let current = vessel@tide;
  let sibling = vessel@ember;
  old_copy;
  current;
  sibling;
  unit
}
