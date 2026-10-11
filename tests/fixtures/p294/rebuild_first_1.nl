struct Node {
    link: Option<ptr<Node>>,
    payload: u8,
}
struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
struct Vessel { first: LiveRoot, }
fn retire(root:LiveRoot)->unit {
    let LiveRoot { p, a, d } = root;
    let vacant = loan_exclusive_read(d) { |end_token| destroy(p, end_token) };
    let whole = erase_slot<Node>(vacant);
    finalize_domain(d);
    deallocate(a, whole);
    unit
}
fn transit(v:Vessel)->Vessel { let Vessel {first: first} = v;
let LiveRoot {p:p_first,a:a_first,d:d_first}=first;
return Vessel {first:LiveRoot{p:p_first,a:a_first,d:d_first}}; }
fn main()->unit {
match try_allocate_one<Node>() {
None => {

unit
},
Some(bundle_0) => {
let OneBacking { allocation, raw } = bundle_0;
let a_0 = allocation;
let vacant_0 = into_slot<Node>(raw);
let d_0 = lifetime_domain();
let p_0 = loan_read(d_0) { |stable_0|
  initialize(vacant_0, Node {
    link: Option<ptr<Node>>::None,
    payload: u8(10)
  }, stable_0)
};
let vessel = Vessel { first: LiveRoot { p: p_0, a: a_0, d: d_0 } };
let shipped = transit(vessel);
let Vessel {first: keeper_0 } = shipped;
retire(keeper_0);
unit
},
}
}
