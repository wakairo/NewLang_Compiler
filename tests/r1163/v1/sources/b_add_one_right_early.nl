struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(173), ember: u8(61) };
  let result = loop (cursor = usize(12)) {
    if (cursor < usize(13)) {
      let step = cursor + usize(1);
      continue(step);
    } else {
      break cursor;
    }
  };
  let result_copy = result;
  let old = if (result_copy == usize(14)) {
    loan_write(vessel@ember) { |slot|
      replace(slot, u8(229))
    }
  } else {
    return unit;
  };
  let old_copy = old;
  let current = vessel@ember;
  let sibling = vessel@tide;
  old_copy;
  current;
  sibling;
  unit
}
