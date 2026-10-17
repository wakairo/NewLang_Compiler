//! DLA-2 pinned Rust intrusive-collections=0.10.3, whole-operation O(1) removal.
use intrusive_collections::{intrusive_adapter, LinkedList, LinkedListLink};
use std::cell::Cell;
use std::ptr::NonNull;
use std::rc::Rc;
use std::sync::atomic::{AtomicU64, Ordering};

static NEXT_POOL: AtomicU64 = AtomicU64::new(1);
intrusive_adapter!(NodeAdapter = Box<Node>: Node { link => LinkedListLink });

pub struct Node { link: LinkedListLink, value: u32, drops: Rc<Cell<usize>> }
impl Node {
    pub fn boxed(value: u32, drops: &Rc<Cell<usize>>) -> Box<Self> {
        Box::new(Self { link: LinkedListLink::new(), value, drops: drops.clone() })
    }
    pub fn value(&self) -> u32 { self.value }
}
impl Drop for Node {
    fn drop(&mut self) { self.drops.set(self.drops.get() + 1); }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Handle { pool: u64, slot: usize, generation: u64, address: usize }
impl Handle { pub fn original_address(&self) -> usize { self.address } }
struct Slot { ptr: Option<NonNull<Node>>, generation: u64 }
pub struct List {
    list: LinkedList<NodeAdapter>,
    slots: Vec<Slot>,
    pool_id: u64,
    remove_slot_probes: usize,
}
impl List {
    pub fn new() -> Self {
        let id = NEXT_POOL.fetch_add(1, Ordering::Relaxed);
        assert!(id != 0 && id < u64::MAX);
        Self {
            list: LinkedList::new(NodeAdapter::new()),
            slots: Vec::new(), pool_id: id, remove_slot_probes: 0,
        }
    }
    // AMORTIZED O(1); slot Vec growth may allocate/copy metadata.
    // Underlying original Box<Node> is NEVER moved/rebuilt.
    pub fn append(&mut self, node: Box<Node>) -> Handle {
        let ptr = NonNull::from(node.as_ref());
        let index = self.slots.len();
        self.slots.push(Slot { ptr: Some(ptr), generation: 1 });
        self.list.push_back(node);
        Handle { pool: self.pool_id, slot: index,
                 generation: 1, address: ptr.as_ptr() as usize }
    }
    // Entire known-live handle removal is WORST-CASE O(1) in list n.
    // No scan, hash, allocation or Vec::remove/shift.
    pub fn detach_known(&mut self, handle: Handle) -> Option<Box<Node>> {
        if handle.pool != self.pool_id { return None; }
        self.remove_slot_probes += 1;
        let slot = self.slots.get_mut(handle.slot)?;
        if slot.generation != handle.generation { return None; }
        let ptr = slot.ptr?;
        if (ptr.as_ptr() as usize) != handle.address { return None; }
        // SAFETY: ptr came from original Box inserted by this List::append;
        // private Slot.ptr is Some iff this Box remains linked in THIS list.
        // No safe API can edit Node.link, registry, or reenter under &mut self.
        // Foreign/invalid inputs were checked *without dereferencing* address.
        let owner = unsafe { self.list.cursor_mut_from_ptr(ptr.as_ptr()).remove() }?;
        slot.ptr = None;
        slot.generation = slot.generation.checked_add(1).expect("generation exhausted");
        Some(owner) // the same original Box transferred, not newly allocated
    }
    pub fn delete_known(&mut self, handle: Handle) -> bool {
        self.detach_known(handle).is_some()
    }
    pub fn probes(&self) -> usize { self.remove_slot_probes }
    pub fn values(&self) -> Vec<u32> {
        // O(n) test oracle OUTSIDE the operation.
        self.list.iter().map(Node::value).collect()
    }
}
impl Default for List { fn default() -> Self { Self::new() } }

fn last_delete_append() {
    let drops=Rc::new(Cell::new(0));let mut list=List::new();
    let a=list.append(Node::boxed(1,&drops));
    let b=list.append(Node::boxed(2,&drops));
    assert!(list.delete_known(b));assert_eq!(drops.get(),1);
    assert!(!list.delete_known(b));
    let c=list.append(Node::boxed(3,&drops));
    assert_eq!(list.values(),vec![1,3]);
    assert_ne!(a.original_address(),c.original_address());
    drop(list);assert_eq!(drops.get(),3);
}
fn preserve_and_reparent() {
    let drops=Rc::new(Cell::new(0));let mut left=List::new();
    let a=left.append(Node::boxed(1,&drops));
    let b=left.append(Node::boxed(2,&drops));
    let c=left.append(Node::boxed(3,&drops));
    let original=b.original_address();
    let owner=left.detach_known(b).expect("middle B live");
    assert_eq!((&*owner as *const Node) as usize,original);
    assert_eq!(owner.value(),2);assert_eq!(drops.get(),0);
    assert_eq!(left.values(),vec![1,3]);
    assert!(left.detach_known(b).is_none());
    let mut right=List::new();
    assert!(right.detach_known(b).is_none()); // foreign pool
    let b2=right.append(owner);
    assert_eq!(b2.original_address(),original);
    assert_eq!(right.values(),vec![2]);
    assert!(left.detach_known(a).is_some());
    assert!(left.detach_known(c).is_some());
    assert!(left.values().is_empty());
    drop(left);drop(right);assert_eq!(drops.get(),3);
}
fn scale() {
    for &n in &[3_usize,16,256] {
        let drops=Rc::new(Cell::new(0));let mut list=List::new();
        let mut tokens=Vec::new();
        for i in 0..n { tokens.push(list.append(Node::boxed(i as u32,&drops))); }
        let h=tokens[n/2];
        let baseline=list.probes();
        let owner=list.detach_known(h).expect("known original");
        assert_eq!(list.probes()-baseline,1);
        assert_eq!((&*owner as *const Node) as usize,h.original_address());
        assert_eq!(owner.value(),(n/2) as u32);
        assert_eq!(list.values().len(),n-1);
        let baseline=list.probes();
        assert!(list.detach_known(h).is_none());
        assert_eq!(list.probes()-baseline,1);
        drop(owner);drop(list);assert_eq!(drops.get(),n);
        println!("DLA2_SCALE n={n} live_probes=1 stale_probes=1 original_box=true frees={n}");
    }
}
fn invalid_handles() {
    let drops=Rc::new(Cell::new(0));
    let mut a=List::new();let mut b=List::new();
    let h=a.append(Node::boxed(9,&drops));
    assert!(b.detach_known(h).is_none());
    assert_eq!(b.probes(),0);
    let mut wrong=h;wrong.generation+=1;
    assert!(a.detach_known(wrong).is_none());
    wrong=h;wrong.address^=16;
    assert!(a.detach_known(wrong).is_none());
    wrong=h;wrong.slot=usize::MAX;
    assert!(a.detach_known(wrong).is_none());
    assert_eq!(a.values(),vec![9]);
    assert_eq!(drops.get(),0);
    drop(a);drop(b);assert_eq!(drops.get(),1);
}
fn run(){last_delete_append();preserve_and_reparent();scale();invalid_handles();}
fn main(){
    run();
    println!("DLA2_INTRUSIVE_O1_OK original_box; true_links; safe_slot_guard; stale_foreign; owner_reparent; n3_16_256");
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]fn historical_last_delete(){last_delete_append();}
    #[test]fn exact_owner_reparent(){preserve_and_reparent();}
    #[test]fn bounded_lookup_at_n(){scale();}
    #[test]fn invalid_foreign_guards(){invalid_handles();}
}
