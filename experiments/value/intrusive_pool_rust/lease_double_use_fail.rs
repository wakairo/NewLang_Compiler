// V552 compile-negative: deliberately use the SAME nonCopy DetachedLease twice.
// Expect rustc E0382 (use of moved value), not an unresolved name or syntax error.
#[allow(dead_code)]
#[path = "intrusive_pool.rs"]
mod baseline;

fn main() {
    let mut pool = baseline::intrusive::Pool::new(baseline::intrusive::Tracker::default());
    let lease = pool.take_free().expect("original A");
    let _active = match pool.attach_active(lease) {
        Ok(active) => active,
        Err(_) => panic!("unexpected error"),
    };
    // This MUST be a static move error, not runtime stale-locator rejection.
    let _second_use = pool.recycle_free(lease);
}
