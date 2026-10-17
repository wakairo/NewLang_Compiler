// DLA-1 FROZEN Rust AFTER real cJSON 2020 last-detach / append maintenance.
 // Natural std-only alternative; compare semantic effects, not arbitrary C prev/next layout.
#![allow(dead_code)]
use std::cell::Cell;
use std::rc::Rc;
use std::sync::atomic::{AtomicU64, Ordering};

static NEXT_ID: AtomicU64 = AtomicU64::new(1);

#[derive(Debug)]
pub struct Node {
    id: u64,
    value: u8,
    destroyed: Rc<Cell<usize>>,
}
impl Drop for Node {
    fn drop(&mut self) {
        self.destroyed.set(self.destroyed.get() + 1);
    }
}
impl Node {
    pub fn boxed(value: u8, destroyed: &Rc<Cell<usize>>) -> Box<Self> {
        Box::new(Self {
            id: NEXT_ID.fetch_add(1, Ordering::Relaxed),
            value,
            destroyed: destroyed.clone(),
        })
    }
}
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Handle {
    id: u64,
    address: usize,
}
pub struct OrderedNodes {
    nodes: Vec<Box<Node>>, // original boxes; Vec moves ONLY owning handles
}
impl OrderedNodes {
    pub fn new() -> Self { Self { nodes: Vec::new() } }
    pub fn append(&mut self, node: Box<Node>) -> Handle {
        let handle = Handle { id: node.id, address: (&*node as *const Node) as usize };
        self.nodes.push(node);
        handle
    }
    pub fn values(&self) -> Vec<u8> {
        self.nodes.iter().map(|x| x.value).collect()
    }
    pub fn get(&self, handle: Handle) -> Option<&Node> {
        self.nodes.iter().map(Box::as_ref).find(|n|
            n.id == handle.id && (std::ptr::from_ref(*n) as usize) == handle.address)
    }

    // NEW public maintenance operations. Return EXACT original owner Box,
    // so a removed-but-detached Node can outlive its old list and be re-adopted.
    pub fn detach_at(&mut self, index: usize) -> Option<Box<Node>> {
        if index >= self.nodes.len() { None } else { Some(self.nodes.remove(index)) }
    }

    // Corresponds to cJSON_DeleteItemFromArray: consume and destroy original.
    pub fn delete_at(&mut self, index: usize) -> bool {
        self.detach_at(index).is_some() // temporary Box Drop, exactly once
    }

    // Pointer-identity-equivalent locator API. Check original identity before
    // any reference, not C's raw pointer dereference; generation/id rejects ABA.
    // Lookup O(n); C's valid live pointer detach can be O(1).
    pub fn detach_by_handle(&mut self, handle: Handle) -> Option<Box<Node>> {
        let index = self.nodes.iter().position(|node|
            node.id == handle.id && (&**node as *const Node as usize) == handle.address)?;
        self.detach_at(index)
    }
}
fn previous_oracle() {
    let frees = Rc::new(Cell::new(0));
    let mut nodes = OrderedNodes::new();
    let a = nodes.append(Node::boxed(b'A', &frees));
    let b = nodes.append(Node::boxed(b'B', &frees));
    assert_eq!(nodes.values(), b"AB".to_vec());
    assert!(nodes.get(a).is_some());
    assert!(nodes.get(b).is_some());
    assert_ne!(a.address, b.address);
    drop(nodes);
    assert_eq!(frees.get(), 2);
}
fn delete_last_append_oracle() {
    let frees = Rc::new(Cell::new(0));
    let mut nodes = OrderedNodes::new();
    let original_a = nodes.append(Node::boxed(b'A', &frees));
    let stale_b = nodes.append(Node::boxed(b'B', &frees));
    assert!(nodes.delete_at(1)); // B original genuinely Drop/EndRoot
    assert_eq!(frees.get(), 1);
    assert!(nodes.get(stale_b).is_none());
    let original_c = nodes.append(Node::boxed(b'C', &frees));
    assert_eq!(nodes.values(), b"AC".to_vec());
    assert_eq!(nodes.get(original_a).unwrap().value, b'A');
    assert_eq!(nodes.get(original_c).unwrap().value, b'C');
    assert!(nodes.get(stale_b).is_none()); // even if allocator reused address
    drop(nodes);
    assert_eq!(frees.get(), 3);
}
fn detach_preserve_oracle() {
    let frees = Rc::new(Cell::new(0));
    let mut a = OrderedNodes::new();
    let a_h = a.append(Node::boxed(b'A', &frees));
    let b_h = a.append(Node::boxed(b'B', &frees));
    let c_h = a.append(Node::boxed(b'C', &frees));
    let original_address = b_h.address;
    let detached = a.detach_by_handle(b_h).expect("middle original B");
    assert_eq!((&*detached as *const Node) as usize, original_address);
    assert_eq!(a.values(), b"AC".to_vec());
    assert!(a.get(b_h).is_none());
    assert!(!a.delete_at(99)); // invalid index is ordinary total result
    let mut other = OrderedNodes::new();
    let adopted = other.append(detached); // exact original Box moved
    assert_eq!(adopted, b_h);
    assert_eq!(other.get(b_h).unwrap().value, b'B');
    assert_eq!(a.get(a_h).unwrap().value, b'A');
    assert_eq!(a.get(c_h).unwrap().value, b'C');
    assert_eq!(frees.get(), 0);
    drop(a);
    drop(other);
    assert_eq!(frees.get(), 3);
}
fn head_last_singleton_oracle() {
    let frees = Rc::new(Cell::new(0));
    let mut nodes = OrderedNodes::new();
    let first = nodes.append(Node::boxed(b'A', &frees));
    nodes.append(Node::boxed(b'B', &frees));
    nodes.append(Node::boxed(b'C', &frees));
    let detached_head = nodes.detach_at(0).expect("head");
    assert_eq!(nodes.values(), b"BC".to_vec());
    assert_eq!((&*detached_head as *const Node) as usize, first.address);
    let detached_last = nodes.detach_at(1).expect("last");
    assert_eq!(detached_last.value, b'C');
    assert_eq!(nodes.values(), b"B".to_vec());
    nodes.append(detached_head);
    nodes.append(detached_last);
    assert_eq!(nodes.values(), b"BAC".to_vec());
    assert_eq!(frees.get(), 0);
    drop(nodes);
    assert_eq!(frees.get(), 3);

    let frees = Rc::new(Cell::new(0));
    let mut single = OrderedNodes::new();
    let one = single.append(Node::boxed(b'S', &frees));
    let owner = single.detach_at(0).expect("singleton");
    assert!(single.values().is_empty());
    assert_eq!(frees.get(), 0);
    assert!(single.get(one).is_none());
    assert_eq!(single.append(owner), one);
    assert_eq!(single.values(), b"S".to_vec());
    drop(single);
    assert_eq!(frees.get(), 1);
}
fn full_oracle() {
    previous_oracle();
    delete_last_append_oracle();
    detach_preserve_oracle();
    head_last_singleton_oracle();
}
#[cfg(not(test))]
fn main() {
    full_oracle();
    println!("DLA1_RUST_AFTER_OK old_contract; last_delete_append; same_box_detach_reparent; head_middle_last_singleton; frees_exact");
}
#[cfg(test)]
mod tests {
    #[test] fn prior_api_unchanged() { super::previous_oracle(); }
    #[test] fn last_delete_then_append() { super::delete_last_append_oracle(); }
    #[test] fn detach_preserves_original_owner_reparent() { super::detach_preserve_oracle(); }
    #[test] fn head_last_singleton_and_order() { super::head_last_singleton_oracle(); }
}
