// V559 F-1: stable Rust comparison to SQLite 3.46.1 sqlite3_str API.
// No crates, no nightly, no custom unsafe buffer management.
// Fault-inject real System allocator rejection via #[global_allocator] wrapper.
use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicBool, AtomicIsize, AtomicUsize, Ordering};

const OK: i32 = 0;
const NOMEM: i32 = 7;
const TOOBIG: i32 = 18;

struct Injector;
static ARMED: AtomicBool = AtomicBool::new(false);
static NTH: AtomicUsize = AtomicUsize::new(0);
static ONCE: AtomicBool = AtomicBool::new(true);
static ATTEMPTS: AtomicUsize = AtomicUsize::new(0);
static REJECTED: AtomicUsize = AtomicUsize::new(0);
static LIVE: AtomicIsize = AtomicIsize::new(0);

fn reject() -> bool {
    if !ARMED.load(Ordering::SeqCst) { return false; }
    let i = ATTEMPTS.fetch_add(1, Ordering::SeqCst) + 1;
    let n = NTH.load(Ordering::SeqCst);
    if n != 0 && i >= n && (!ONCE.load(Ordering::SeqCst) || i == n) {
        REJECTED.fetch_add(1, Ordering::SeqCst);
        true
    } else { false }
}

// Only instrument allocator rejects here. All unsafe calls are the exact
// System GlobalAlloc forwarding contract (no raw buffer interpretation).
unsafe impl GlobalAlloc for Injector {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if reject() { return std::ptr::null_mut(); }
        let p = unsafe { System.alloc(layout) };
        if !p.is_null() { LIVE.fetch_add(1, Ordering::SeqCst); }
        p
    }
    unsafe fn realloc(&self, p: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        if reject() { return std::ptr::null_mut(); }
        unsafe { System.realloc(p, layout, new_size) }
    }
    unsafe fn dealloc(&self, p: *mut u8, layout: Layout) {
        LIVE.fetch_sub(1, Ordering::SeqCst);
        unsafe { System.dealloc(p, layout) }
    }
}
#[global_allocator]
static ALLOCATOR: Injector = Injector;

fn arm(n: usize, only_nth: bool) {
    ATTEMPTS.store(0, Ordering::SeqCst);
    REJECTED.store(0, Ordering::SeqCst);
    NTH.store(n, Ordering::SeqCst);
    ONCE.store(only_nth, Ordering::SeqCst);
    ARMED.store(true, Ordering::SeqCst);
}
fn disarm() { ARMED.store(false, Ordering::SeqCst); }

struct Builder {
    bytes: Vec<u8>, // zero heap allocation on construction
    error: i32,
    max_len: usize,
}
impl Builder {
    fn new(limit: usize) -> Self { Self { bytes: Vec::new(), error: OK, max_len: limit } }
    fn status(&self) -> i32 { self.error }
    fn length(&self) -> usize {
        if self.bytes.is_empty() { 0 } else { self.bytes.len() - 1 }
    }
    // Append exactly bytes of caller-provided nonzero data. Keeps trailing
    // NUL in the Vec for C-style representation; no hidden CString allocation.
    fn append(&mut self, data: &[u8]) {
        if self.error != OK || data.is_empty() { return; }
        let needed = self.length().checked_add(data.len()).and_then(|n| n.checked_add(1));
        let Some(required) = needed else { self.fail(TOOBIG); return; };
        if required - 1 > self.max_len { self.fail(TOOBIG); return; }
        let extra = required - self.bytes.len();
        if self.bytes.try_reserve_exact(extra).is_err() { self.fail(NOMEM); return; }
        if !self.bytes.is_empty() { self.bytes.pop(); }
        self.bytes.extend_from_slice(data); // capacity pre-reserved
        self.bytes.push(0); // capacity pre-reserved
    }
    fn fail(&mut self, error: i32) {
        self.error = error;
        self.bytes = Vec::new(); // drop old heap buffer, sticky error
    }
    fn finish(self) -> Option<Vec<u8>> {
        if self.error == OK && !self.bytes.is_empty() { Some(self.bytes) }
        else { None } // conflates valid empty with error, as SQLite does
    }
}

struct Case { name: &'static str, limit: usize, nth: usize, once: bool,
              error: i32, null: bool, rejects: usize, initial: i32 }
const CASES: [Case; 9] = [
    Case {name:"success",limit:usize::MAX,nth:0,once:true,error:OK,null:false,rejects:0,initial:OK},
    Case {name:"empty",limit:usize::MAX,nth:0,once:true,error:OK,null:true,rejects:0,initial:OK},
    Case {name:"limit",limit:32,nth:0,once:true,error:TOOBIG,null:true,rejects:0,initial:OK},
    Case {name:"oom_new_once",limit:usize::MAX,nth:1,once:true,error:NOMEM,null:true,rejects:1,initial:OK},
    Case {name:"oom_grow_once",limit:usize::MAX,nth:2,once:true,error:NOMEM,null:true,rejects:1,initial:OK},
    Case {name:"oom_realloc_once",limit:usize::MAX,nth:3,once:true,error:NOMEM,null:true,rejects:1,initial:OK},
    Case {name:"oom_new_persistent",limit:usize::MAX,nth:1,once:false,error:NOMEM,null:true,rejects:1,initial:OK},
    Case {name:"oom_grow_persistent",limit:usize::MAX,nth:2,once:false,error:NOMEM,null:true,rejects:1,initial:OK},
    Case {name:"oom_realloc_persistent",limit:usize::MAX,nth:3,once:false,error:NOMEM,null:true,rejects:1,initial:OK},
];
fn run_case(c: &Case) {
    let before = LIVE.load(Ordering::SeqCst);
    arm(c.nth, c.once);
    let mut s = Builder::new(c.limit);
    let first = s.status(); // Rust ctor infallible; differs from C OOM singleton timing
    if c.name != "empty" {
        s.append(b"ab"); s.append(b"cd");
        s.append(&[b'X'; 128]);
    }
    let status = s.status();
    let length = s.length();
    let attempts = ATTEMPTS.load(Ordering::SeqCst);
    let rejected = REJECTED.load(Ordering::SeqCst);
    let owned = s.finish();
    disarm();
    assert_eq!(status,c.error,"{} status",c.name);
    assert_eq!(first,c.initial,"{} initial",c.name);
    assert_eq!(owned.is_none(),c.null,"{} null",c.name);
    assert_eq!(rejected,c.rejects,"{} rejections",c.name);
    if let Some(v) = &owned {
        assert_eq!(v.len(),133);
        assert_eq!(&v[..4],b"abcd");
        assert!(v[4..132].iter().all(|x| *x == b'X'));
        assert_eq!(v[132],0);
    }
    drop(owned);
    assert_eq!(LIVE.load(Ordering::SeqCst),before,"{} cleanup",c.name);
    // after rejection is disabled, the host can create a *new* working builder
    let mut later = Builder::new(usize::MAX);
    later.append(b"Z");
    assert_eq!(later.status(),OK);
    assert_eq!(later.finish().as_deref(),Some(&b"Z\0"[..]));
    assert_eq!(LIVE.load(Ordering::SeqCst),before,"{} later cleanup",c.name);
    println!("RUST_CASE {} code={} null={} attempts={} rejected={} initial={} len={} live_delta=0 later=OK",c.name,status,c.null as i32,attempts,rejected,first,length);
}
fn main() {
    for c in CASES.iter() { run_case(c); }
    println!("RUST_FALLIBLE_STR_DONE cases=9");
}
