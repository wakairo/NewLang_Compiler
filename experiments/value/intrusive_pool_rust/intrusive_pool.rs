// V552 same-original intrusive pool. Only internal raw-pointer code is unsafe.
// Standalone: rustc --edition=2021 -D warnings --test intrusive_pool.rs -o tests && ./tests
//             rustc --edition=2021 -D warnings intrusive_pool.rs -o demo && ./demo
use std::cell::{Cell, UnsafeCell};
use std::marker::PhantomPinned;
use std::pin::Pin;
use std::ptr;
use std::rc::{Rc, Weak};

pub mod intrusive {
    use super::*;

    const COUNT: usize = 3;

    #[derive(Copy, Clone, Eq, PartialEq, Debug)]
    enum State { Free, Detached, Active }

    #[repr(C)]
    struct Link {
        prev: *mut Link,
        next: *mut Link,
        _pin: PhantomPinned,
    }
    impl Link {
        fn blank() -> Self { Self { prev: ptr::null_mut(), next: ptr::null_mut(), _pin: PhantomPinned } }
    }

    #[repr(C)]
    struct Node {
        // UnsafeCell explicitly authorizes link writes through raw pointers
        // obtained from a shared reference; no safe caller sees this place.
        link: UnsafeCell<Link>,
        id: u8,
        state: Cell<State>,
        generation: Cell<u64>,
        freed: Rc<Cell<usize>>,
        _pin: PhantomPinned,
    }
    impl Node {
        fn new(id: u8, tracker: &Tracker) -> Self {
            tracker.created.set(tracker.created.get() + 1);
            Self {
                link: UnsafeCell::new(Link::blank()),
                id,
                state: Cell::new(State::Free),
                generation: Cell::new(0),
                freed: tracker.freed.clone(),
                _pin: PhantomPinned,
            }
        }
    }
    impl Drop for Node {
        fn drop(&mut self) { self.freed.set(self.freed.get() + 1); }
    }

    #[derive(Clone, Default)]
    pub struct Tracker { created: Rc<Cell<usize>>, freed: Rc<Cell<usize>> }
    impl Tracker {
        pub fn created(&self) -> usize { self.created.get() }
        pub fn freed(&self) -> usize { self.freed.get() }
    }

    // A membership receipt, not a Box owner. NonCopy and nonClone.
    #[derive(Debug)]
    pub struct DetachedLease {
        slot: usize,
        generation: u64,
        pool: Weak<()>,
    }

    // Cloneable *locator only*. Validation precedes all raw dereferences.
    #[derive(Clone, Debug)]
    pub struct ActiveLocator {
        slot: usize,
        generation: u64,
        pool: Weak<()>,
    }

    #[derive(Debug, Copy, Clone, Eq, PartialEq)]
    pub struct Invalid;

    #[derive(Debug, PartialEq, Eq)]
    pub struct Audit {
        pub free: Vec<u8>,
        pub active: Vec<u8>,
        pub detached: Vec<u8>,
    }

    pub struct Pool {
        free_head: Pin<Box<UnsafeCell<Link>>>,
        active_head: Pin<Box<UnsafeCell<Link>>>,
        // The owning registry is for genuine deallocation and pointer-validation
        // only. Membership traversal actually follows embedded prev/next.
        nodes: [Pin<Box<Node>>; COUNT],
        identity: Rc<()>,
    }

    // All raw link writes occur in this private author boundary.
    // SAFETY preconditions: p denotes a live pinned initialized Link; no
    // simultaneously accessible safe &mut Link or external raw field writers.
    unsafe fn self_link(p: *mut Link) {
        unsafe { (*p).prev = p; (*p).next = p; }
    }

    // SAFETY: p is a live member of one circular list; its prev/next are
    // initialized, live pinned Links in that list, and have reciprocal edges.
    // Pool &mut exclusivity and private fields exclude ordinary safe mutation.
    unsafe fn unlink(p: *mut Link) {
        unsafe {
            let before = (*p).prev;
            let after = (*p).next;
            debug_assert!(!before.is_null() && !after.is_null());
            debug_assert_eq!((*before).next, p);
            debug_assert_eq!((*after).prev, p);
            (*before).next = after;
            (*after).prev = before;
            self_link(p);
        }
    }

    // SAFETY: head is a live pinned initialized circular sentinel, and p is
    // a live pinned self-linked detached Node; p is not linked elsewhere.
    unsafe fn insert_tail(head: *mut Link, p: *mut Link) {
        unsafe {
            let before = (*head).prev;
            debug_assert_eq!((*before).next, head);
            debug_assert_eq!((*p).prev, p);
            debug_assert_eq!((*p).next, p);
            (*p).prev = before;
            (*p).next = head;
            (*before).next = p;
            (*head).prev = p;
        }
    }

    impl Pool {
        // Box::pin is an infallible API with process-abort on allocation failure;
        // NO Err/OOM-world equivalence to a fallible C constructor is claimed.
        pub fn new(tracker: Tracker) -> Self {
            let pool = Self {
                free_head: Box::pin(UnsafeCell::new(Link::blank())),
                active_head: Box::pin(UnsafeCell::new(Link::blank())),
                nodes: std::array::from_fn(|i| Box::pin(Node::new(b'A' + i as u8, &tracker))),
                identity: Rc::new(()),
            };
            // SAFETY: all five Box allocations have reached stable final pinned
            // pointee addresses before initializing any self-referential links.
            unsafe {
                self_link(pool.free_ptr());
                self_link(pool.active_ptr());
                for i in 0..COUNT {
                    let p = pool.node_ptr(i);
                    self_link(p);
                    insert_tail(pool.free_ptr(), p);
                }
            }
            pool
        }

        fn free_ptr(&self) -> *mut Link { self.free_head.as_ref().get_ref().get() }
        fn active_ptr(&self) -> *mut Link { self.active_head.as_ref().get_ref().get() }
        fn node(&self, i: usize) -> &Node { self.nodes[i].as_ref().get_ref() }
        fn node_ptr(&self, i: usize) -> *mut Link { self.node(i).link.get() }

        fn from_link(&self, ptr: *mut Link) -> Option<usize> {
            (0..COUNT).find(|&i| self.node_ptr(i) == ptr)
        }
        fn matches_pool(&self, weak: &Weak<()>) -> bool {
            weak.upgrade().is_some_and(|actual| Rc::ptr_eq(&actual, &self.identity))
        }
        fn valid_lease(&self, lease: &DetachedLease) -> bool {
            self.matches_pool(&lease.pool)
                && lease.slot < COUNT
                && self.node(lease.slot).state.get() == State::Detached
                && self.node(lease.slot).generation.get() == lease.generation
                && self.self_linked(lease.slot)
        }
        fn self_linked(&self, i: usize) -> bool {
            let p = self.node_ptr(i);
            // SAFETY: only an actual original, live, pinned Node link is used;
            // no pointer sourced from an untrusted caller is dereferenced.
            unsafe { (*p).prev == p && (*p).next == p }
        }

        pub fn take_free(&mut self) -> Option<DetachedLease> {
            let head = self.free_ptr();
            // SAFETY: head is a live original pinned sentinel, private, and
            // all established list transitions preserve head initialization.
            let first = unsafe { (*head).next };
            if first == head { return None; }
            // Validate against ORIGINAL owner registry *before* further deref.
            let i = self.from_link(first).expect("internal free list node must be original");
            assert_eq!(self.node(i).state.get(), State::Free);
            // SAFETY: first is a live original member of this free head; both
            // reciprocal neighbors are live from the pool-wide list invariant.
            unsafe { unlink(first); }
            let n = self.node(i);
            n.state.set(State::Detached);
            n.generation.set(n.generation.get() + 1);
            Some(DetachedLease {
                slot: i, generation: n.generation.get(),
                pool: Rc::downgrade(&self.identity),
            })
        }

        pub fn attach_active(&mut self, lease: DetachedLease)
            -> Result<ActiveLocator, DetachedLease> {
            if !self.valid_lease(&lease) { return Err(lease); }
            let p = self.node_ptr(lease.slot);
            // SAFETY: checked original registry, original still alive,
            // detached has self-linked original p, active head live/pinned.
            unsafe { insert_tail(self.active_ptr(), p); }
            self.node(lease.slot).state.set(State::Active);
            Ok(ActiveLocator {
                slot: lease.slot, generation: lease.generation,
                pool: Rc::downgrade(&self.identity),
            })
        }

        pub fn is_active(&self, locator: &ActiveLocator) -> bool {
            // A cloneable locator is NOT borrowed access or membership rights.
            self.matches_pool(&locator.pool)
                && locator.slot < COUNT
                && self.node(locator.slot).state.get() == State::Active
                && self.node(locator.slot).generation.get() == locator.generation
        }

        pub fn detach_active(&mut self, locator: &ActiveLocator)
            -> Result<DetachedLease, Invalid> {
            if !self.is_active(locator) { return Err(Invalid); }
            let p = self.node_ptr(locator.slot);
            // SAFETY: an original active Node still linked under active_head,
            // with established reciprocal original neighbor links.
            unsafe { unlink(p); }
            let n = self.node(locator.slot);
            n.state.set(State::Detached);
            n.generation.set(n.generation.get() + 1);
            Ok(DetachedLease {
                slot: locator.slot, generation: n.generation.get(),
                pool: Rc::downgrade(&self.identity),
            })
        }

        pub fn recycle_free(&mut self, lease: DetachedLease)
            -> Result<(), DetachedLease> {
            if !self.valid_lease(&lease) { return Err(lease); }
            let p = self.node_ptr(lease.slot);
            // SAFETY: original detached Node and free sentinel live/pinned.
            unsafe { insert_tail(self.free_ptr(), p); }
            let n = self.node(lease.slot);
            n.state.set(State::Free);
            n.generation.set(n.generation.get() + 1);
            Ok(())
        }

        // For testing: numeric identity snapshots confer no pointer access.
        pub fn originals(&self) -> [(usize, usize); COUNT] {
            std::array::from_fn(|i| (self.node(i) as *const Node as usize, self.node_ptr(i) as usize))
        }
        pub fn sentinels(&self) -> (usize, usize) {
            (self.free_ptr() as usize, self.active_ptr() as usize)
        }
        pub fn owner_key(&self) -> usize { Rc::as_ptr(&self.identity) as usize }

        fn chain(&self, head: *mut Link, state: State, seen: &mut [bool; COUNT]) -> Vec<u8> {
            let mut out = Vec::new();
            let mut before = head;
            // SAFETY: original pinned head was fully initialized before
            // exposure; each next hop is checked for membership in the
            // original owner registry before dereferencing that hop.
            let mut p = unsafe { (*head).next };
            for _ in 0..=COUNT {
                if p == head {
                    // SAFETY: head is pinned live; before either head or
                    // previously registry-validated live original.
                    unsafe {
                        assert_eq!((*head).prev, before);
                        assert_eq!((*before).next, head);
                    }
                    return out;
                }
                let i = self.from_link(p).expect("non-original link must not be followed");
                assert!(!seen[i], "repeated physical original in list");
                seen[i] = true;
                let n = self.node(i);
                assert_eq!(n.state.get(), state);
                // SAFETY: p and before now both proven live original or head;
                // no mutable API can run while this &self audit is active.
                unsafe {
                    assert_eq!((*p).prev, before);
                    assert_eq!((*before).next, p);
                }
                out.push(n.id);
                before = p;
                // SAFETY: p is original live pinned Link from checked registry.
                p = unsafe { (*p).next };
            }
            panic!("nonterminating intrusive list");
        }

        pub fn audit(&self) -> Audit {
            let mut seen = [false; COUNT];
            let free = self.chain(self.free_ptr(), State::Free, &mut seen);
            let active = self.chain(self.active_ptr(), State::Active, &mut seen);
            let mut detached = Vec::new();
            for (i, used) in seen.iter().enumerate() {
                if !used {
                    assert_eq!(self.node(i).state.get(), State::Detached);
                    assert!(self.self_linked(i));
                    detached.push(self.node(i).id);
                }
            }
            Audit { free, active, detached }
        }

        // A recoverable Busy returns this exact original Pool responsibility.
        // Successful return drops Pool/three original Nodes exactly once via RAII.
        pub fn finish(self) -> Result<(), Self> {
            let audit = self.audit();
            if audit.active.is_empty() && audit.detached.is_empty() { Ok(()) }
            else { Err(self) }
        }
    }
}

pub fn format_node_id(id: u8) -> String { format!("node-{id}") }

fn scenario() {
    use intrusive::{Pool, Tracker};
    let tracker = Tracker::default();
    let mut pool = Pool::new(tracker.clone());
    assert_eq!(tracker.created(), 3);
    assert_eq!(tracker.freed(), 0);
    let physical = pool.originals();
    for i in 0..3 {
        assert_eq!(physical[i].0, physical[i].1); // repr(C) first embedded Link starts at Node address
        for j in (i + 1)..3 { assert_ne!(physical[i].0, physical[j].0); }
    }
    let heads = pool.sentinels();
    assert_ne!(heads.0, heads.1);
    assert_eq!(pool.audit().free, b"ABC".to_vec());
    assert!(pool.audit().active.is_empty());

    let a = pool.take_free().expect("take original A");
    let active_a = pool.attach_active(a).expect("activate A");
    let copied_a = active_a.clone(); // locator copy is permitted, not a lease copy
    assert!(pool.is_active(&copied_a));
    let b = pool.take_free().expect("take original B");
    let active_b = pool.attach_active(b).expect("activate B");
    let c = pool.take_free().expect("take original C");
    assert!(pool.take_free().is_none()); // EMPTY
    assert_eq!(pool.audit().active, b"AB".to_vec());
    assert_eq!(pool.audit().detached, b"C".to_vec());

    let owner_before = pool.owner_key();
    let mut pool = match pool.finish() {
        Err(original) => original,
        Ok(()) => panic!("finish must report Busy and refund Pool"),
    };
    assert_eq!(pool.owner_key(), owner_before);
    assert_eq!(pool.originals(), physical);
    assert_eq!(pool.sentinels(), heads);
    assert_eq!(tracker.freed(), 0);

    pool.recycle_free(c).expect("recycle C");
    let a = pool.detach_active(&active_a).expect("detach same original A");
    let before_invalid = pool.audit();
    assert!(!pool.is_active(&copied_a));
    assert!(matches!(pool.detach_active(&copied_a), Err(intrusive::Invalid)));
    assert_eq!(pool.audit(), before_invalid); // stale clone: Invalid, ZERO mutation
    pool.recycle_free(a).expect("recycle A");
    let b = pool.detach_active(&active_b).expect("detach same original B");
    pool.recycle_free(b).expect("recycle B");
    assert_eq!(pool.audit().free, b"CAB".to_vec());
    assert!(pool.audit().active.is_empty());
    assert_eq!(pool.originals(), physical);
    assert_eq!(pool.sentinels(), heads);
    assert_eq!(tracker.created(), 3); // no new Node allocations on transitions
    pool.finish().unwrap_or_else(|_| panic!("empty finish must succeed"));
    assert_eq!(tracker.freed(), 3); // exactly original A/B/C once
    assert_eq!(format_node_id(b'A'), "node-65");
}

#[cfg(not(test))]
fn main() {
    scenario();
    println!("V552_RUST_INTRUSIVE_OK original3; two_heads; stale_invalid; busy_refund; empty; free3");
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn same_original_pool_full_oracle() { scenario(); }

    #[test]
    fn safe_early_drop_does_not_follow_dangling_links() {
        let tracker = intrusive::Tracker::default();
        let mut pool = intrusive::Pool::new(tracker.clone());
        let lease = pool.take_free().expect("detached");
        drop(pool); // Rust safe RAII; Pool original Box ownership remains unique
        drop(lease); // inert Weak token contains no borrowed raw pointer
        assert_eq!(tracker.freed(), 3);
    }
}
