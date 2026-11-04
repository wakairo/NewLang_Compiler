struct Vessel { ember: u8, tide: u8, }
fn main() -> unit {
  let vessel = Vessel { tide: u8(163), ember: u8(97) };
  let result = loop (cursor = usize(31)) {
    if (usize(28) < cursor) {
      let step = cursor - usize(1);
      continue(step);
    } else {
      break cursor;
    }
  };
  let result_copy = result;
  let old = if (result_copy == usize(28)) {
    loan_write(vessel@tide) { |slot|
      replace(slot, u8(239))
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
