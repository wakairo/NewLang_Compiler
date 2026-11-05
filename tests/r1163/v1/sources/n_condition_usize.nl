struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(157), ember: u8(43) };
  let result = loop (cursor = usize(20)) {
    if (cursor < usize(23)) {
      let step = cursor + usize(1);
      continue(step);
    } else {
      break cursor;
    }
  };
  let result_copy = result;
  let old = if (result_copy) {
    return unit;
  } else {
    loan_write(vessel@ember) { |slot|
      replace(slot, u8(197))
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
