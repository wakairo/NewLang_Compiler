// Track V #536: same-topology 4 tagged slots; ONLY safe Rust.
// Reproduce: rustc --edition=2021 -D warnings --test ring4.rs -o ring4_tests && ./ring4_tests
//            rustc --edition=2021 -D warnings ring4.rs -o ring4 && ./ring4
use std::cell::Cell;
use std::collections::BTreeSet;
use std::rc::Rc;

#[derive(Debug)]
struct OwnedFrame {
    id: u8,
    payload: Box<[u8; 16]>, // one distinct real heap allocation
    released: Rc<Cell<usize>>,
}

impl OwnedFrame {
    fn new(id: u8, released: &Rc<Cell<usize>>) -> Self {
        Self { id, payload: Box::new([id; 16]), released: Rc::clone(released) }
    }
    fn address(&self) -> usize {
        self.payload.as_ptr() as usize // snapshot only while still owned/live
    }
}

impl Drop for OwnedFrame {
    fn drop(&mut self) {
        self.released.set(self.released.get() + 1);
        // Box<[u8;16]> is automatically released after this method.
    }
}

mod tagged_ring {
    const CAP: usize = 4;

    // Fields are private to this module's API: no raw slot / head setter.
    pub struct Ring4<T> {
        slots: [Option<T>; CAP],
        head: usize,
        len: usize,
    }

    impl<T> Ring4<T> {
        pub fn new() -> Self {
            Self { slots: std::array::from_fn(|_| None), head: 0, len: 0 }
        }

        pub fn push(&mut self, value: T) -> Result<(), T> {
            if self.len == CAP { return Err(value); } // EXACT input T returned on FULL
            let tail = (self.head + self.len) % CAP;
            if self.slots[tail].is_some() { return Err(value); } // fail closed if invariant corrupted
            self.slots[tail] = Some(value); // one non-Copy move; no clone
            self.len += 1;
            Ok(())
        }

        pub fn pop(&mut self) -> Option<T> {
            if self.len == 0 { return None; }
            let i = self.head;
            let old = self.slots[i].take(); // original T moved, slot becomes None
            if old.is_some() {
                self.head = (self.head + 1) % CAP;
                self.len -= 1;
            }
            old // invalid FIFO metadata might make wrong output, not memory UB
        }

        pub fn front(&self) -> Option<&T> {
            if self.len == 0 { None } else { self.slots[self.head].as_ref() }
        }

        pub fn finish(self) -> Result<(), Self> {
            if self.len == 0 && self.slots.iter().all(Option::is_none) { Ok(()) }
            else { Err(self) } // exact ring (and all original values) refunded
        }
    }

    #[cfg(test)]
    mod internal_tests {
        use super::*;
        #[test]
        fn four_distinct_contiguous_tagged_envelopes_stay_put_during_operations() {
            let mut ring: Ring4<u8> = Ring4::new();
            macro_rules! addresses {
                () => { std::array::from_fn(|i| (&ring.slots[i] as *const Option<u8>) as usize) };
            }
            let before = addresses!();
            for i in 0..CAP-1 {
                assert_eq!(before[i + 1] - before[i], std::mem::size_of::<Option<u8>>());
            }
            ring.push(7).unwrap();
            let after_push = addresses!();
            assert_eq!(before, after_push);
            assert_eq!(ring.pop(), Some(7));
            assert_eq!(before, addresses!());
        }
    }
}

// Pure diagnostic formatting is unrelated to Ring owner/slot invariants.
fn format_id(id: u8) -> String { format!("frame:{id}") }

fn run_scenario() {
    let released = Rc::new(Cell::new(0));
    let (a, b, c, d, e, f) = (
        OwnedFrame::new(b'A', &released), OwnedFrame::new(b'B', &released),
        OwnedFrame::new(b'C', &released), OwnedFrame::new(b'D', &released),
        OwnedFrame::new(b'E', &released), OwnedFrame::new(b'F', &released),
    );
    let original = [a.address(), b.address(), c.address(), d.address(), e.address(), f.address()];
    assert_eq!(BTreeSet::from(original).len(), 6); // all payload heaps simultaneously live
    let mut ring = tagged_ring::Ring4::new();
    assert!(ring.pop().is_none()); // EMPTY
    ring.push(a).unwrap(); ring.push(b).unwrap();
    ring.push(c).unwrap(); ring.push(d).unwrap();
    assert_eq!(ring.front().map(|v| v.id), Some(b'A'));

    let e = ring.push(e).err().expect("FULL must return original E");
    assert_eq!(e.address(), original[4]);
    assert_eq!(released.get(), 0);
    for (wanted, addr) in [(b'A', original[0]), (b'B', original[1])] {
        let out = ring.pop().expect("A/B");
        assert_eq!((out.id, out.address()), (wanted, addr));
        drop(out); // explicit consumer; not necessary for Rust memory safety
    }
    assert_eq!(released.get(), 2);
    ring.push(e).unwrap(); // slot 0, after wrap
    ring.push(f).unwrap(); // slot 1, after wrap
    assert_eq!(ring.front().map(|v| v.id), Some(b'C'));
    for (wanted, addr) in [(b'C', original[2]), (b'D', original[3]),
                            (b'E', original[4]), (b'F', original[5])] {
        let out = ring.pop().expect("C/D/E/F");
        assert_eq!((out.id, out.address()), (wanted, addr));
        drop(out);
    }
    assert!(ring.front().is_none());
    assert!(ring.pop().is_none()); // EMPTY after wrap
    assert!(ring.finish().is_ok()); // ring now contains precisely four None envelopes
    assert_eq!(released.get(), 6); // all six original resources released exactly once
    assert_eq!(format_id(b'A'), "frame:65");
}

#[cfg(not(test))]
fn main() { run_scenario(); println!("RUST_RING4_SCENARIO_OK"); }

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn full_refund_wrap_empty_identity_explicit_finish() { run_scenario(); }
    #[test]
    fn nonempty_finish_refunds_entire_original_ring_and_raii_drop_is_safe() {
        let released = Rc::new(Cell::new(0));
        let mut ring = tagged_ring::Ring4::new();
        let a = OwnedFrame::new(b'A', &released);
        let addr = a.address();
        ring.push(a).unwrap();
        let ring = match ring.finish() { Err(original) => original, Ok(()) => panic!("nonempty") };
        assert_eq!(ring.front().unwrap().address(), addr);
        assert_eq!(released.get(), 0);
        drop(ring); // Rust RAII frees the retained owner even without explicit pop
        assert_eq!(released.get(), 1);
    }
}
