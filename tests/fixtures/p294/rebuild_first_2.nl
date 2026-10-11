struct Node {
    link: Option<ptr<Node>>,
    payload: u8,
}
struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
struct Vessel { first: LiveRoot, second: LiveRoot, }
fn retire(root:LiveRoot)->unit {
    let LiveRoot { p, a, d } = root;
    let vacant = loan_exclusive_read(d) { |end_token| destroy(p, end_token) };
    let whole = erase_slot<Node>(vacant);
    finalize_domain(d);
    deallocate(a, whole);
    unit
}
fn transit(v:Vessel)->Vessel { let Vessel {first: first, second: second} = v;
let LiveRoot {p:p_first,a:a_first,d:d_first}=first;
return Vessel {first:LiveRoot{p:p_first,a:a_first,d:d_first}, second:second}; }
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
match try_allocate_one<Node>() {
None => {
let void_0 = loan_exclusive_read(d_0) { |ender_0| destroy(p_0, ender_0) };
let raw_0 = erase_slot<Node>(void_0);
finalize_domain(d_0);
deallocate(a_0, raw_0);

unit
},
Some(bundle_1) => {
let OneBacking { allocation, raw } = bundle_1;
let a_1 = allocation;
let vacant_1 = into_slot<Node>(raw);
let d_1 = lifetime_domain();
let p_1 = loan_read(d_1) { |stable_1|
  initialize(vacant_1, Node {
    link: Option<ptr<Node>>::None,
    payload: u8(11)
  }, stable_1)
};
let vessel = Vessel { first: LiveRoot { p: p_0, a: a_0, d: d_0 }, second: LiveRoot { p: p_1, a: a_1, d: d_1 } };
let shipped = transit(vessel);
let Vessel {first: keeper_0, second: keeper_1 } = shipped;
retire(keeper_0);
retire(keeper_1);
unit
},
}
},
}
}
