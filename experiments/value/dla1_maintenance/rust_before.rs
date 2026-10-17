// DLA-1 FROZEN Rust BEFORE maintenance episode (small natural std-only list).
// Not cJSON source and not pointer-for-pointer reimplementation of cJSON internals.
// Public operations initially: own heap Node, append, read, stable identity handle.
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
    // Nodes are original heap allocations. Vec may relocate *Box handles*,
    // never the separately allocated Node pointees.
    nodes: Vec<Box<Node>>,
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
}
fn before_oracle() {
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
#[cfg(not(test))]
fn main() {
    before_oracle();
    println!("DLA1_RUST_BEFORE_OK stable_original_boxes; append; safe_get; freed2");
}
#[cfg(test)]
mod tests {
    #[test]
    fn original_list_before_change() { super::before_oracle(); }
}
