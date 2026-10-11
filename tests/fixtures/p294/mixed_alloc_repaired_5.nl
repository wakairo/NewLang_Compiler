struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}
struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
struct Vessel { first: LiveRoot, second: LiveRoot, third: LiveRoot, fourth: LiveRoot, }
fn retire(root:LiveRoot)->unit {
    let LiveRoot { p, a, d } = root;
    let vacant = loan_exclusive_read(d) { |end_token| destroy(p, end_token) };
    let whole = erase_slot<Node>(vacant);
    finalize_domain(d);
    deallocate(a, whole);
    unit
}
fn transit(v:Vessel)->Vessel { return v; }
fn exchange(v:Vessel)->Vessel {
    let Vessel { first: left, second: right, third: third, fourth: fourth } = v;
    let LiveRoot { p: lp, a: la, d: ld } = left;
    let LiveRoot { p: rp, a: ra, d: rd } = right;
    return Vessel {
        first: LiveRoot { p: lp, a: ra, d: ld },
        second: LiveRoot { p: rp, a: la, d: rd },
        third: third, fourth: fourth
    };
}
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
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
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
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
    payload: u8(11)
  }, stable_1)
};
match try_allocate_one<Node>() {
None => {
let void_1 = loan_exclusive_read(d_1) { |ender_1| destroy(p_1, ender_1) };
let raw_1 = erase_slot<Node>(void_1);
finalize_domain(d_1);
deallocate(a_1, raw_1);
let void_0 = loan_exclusive_read(d_0) { |ender_0| destroy(p_0, ender_0) };
let raw_0 = erase_slot<Node>(void_0);
finalize_domain(d_0);
deallocate(a_0, raw_0);

unit
},
Some(bundle_2) => {
let OneBacking { allocation, raw } = bundle_2;
let a_2 = allocation;
let vacant_2 = into_slot<Node>(raw);
let d_2 = lifetime_domain();
let p_2 = loan_read(d_2) { |stable_2|
  initialize(vacant_2, Node {
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
    payload: u8(12)
  }, stable_2)
};
match try_allocate_one<Node>() {
None => {
let void_2 = loan_exclusive_read(d_2) { |ender_2| destroy(p_2, ender_2) };
let raw_2 = erase_slot<Node>(void_2);
finalize_domain(d_2);
deallocate(a_2, raw_2);
let void_1 = loan_exclusive_read(d_1) { |ender_1| destroy(p_1, ender_1) };
let raw_1 = erase_slot<Node>(void_1);
finalize_domain(d_1);
deallocate(a_1, raw_1);
let void_0 = loan_exclusive_read(d_0) { |ender_0| destroy(p_0, ender_0) };
let raw_0 = erase_slot<Node>(void_0);
finalize_domain(d_0);
deallocate(a_0, raw_0);

unit
},
Some(bundle_3) => {
let OneBacking { allocation, raw } = bundle_3;
let a_3 = allocation;
let vacant_3 = into_slot<Node>(raw);
let d_3 = lifetime_domain();
let p_3 = loan_read(d_3) { |stable_3|
  initialize(vacant_3, Node {
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
    payload: u8(13)
  }, stable_3)
};
match try_allocate_one<Node>() {
None => {
let void_3 = loan_exclusive_read(d_3) { |ender_3| destroy(p_3, ender_3) };
let raw_3 = erase_slot<Node>(void_3);
finalize_domain(d_3);
deallocate(a_3, raw_3);
let void_2 = loan_exclusive_read(d_2) { |ender_2| destroy(p_2, ender_2) };
let raw_2 = erase_slot<Node>(void_2);
finalize_domain(d_2);
deallocate(a_2, raw_2);
let void_1 = loan_exclusive_read(d_1) { |ender_1| destroy(p_1, ender_1) };
let raw_1 = erase_slot<Node>(void_1);
finalize_domain(d_1);
deallocate(a_1, raw_1);
let void_0 = loan_exclusive_read(d_0) { |ender_0| destroy(p_0, ender_0) };
let raw_0 = erase_slot<Node>(void_0);
finalize_domain(d_0);
deallocate(a_0, raw_0);

unit
},
Some(bundle_4) => {
let OneBacking { allocation, raw } = bundle_4;
let a_4 = allocation;
let vacant_4 = into_slot<Node>(raw);
let d_4 = lifetime_domain();
let p_4 = loan_read(d_4) { |stable_4|
  initialize(vacant_4, Node {
    next: Option<ptr<Node>>::None,
    prev: Option<ptr<Node>>::None,
    child: Option<ptr<Node>>::None,
    payload: u8(14)
  }, stable_4)
};
let previous_0 = loan_read(d_0) { |guard_0|
  let projected_0 = ref_from_ptr(write, p_0, guard_0);
  replace(projected_0@child, Option<ptr<Node>>::Some(p_1))
};
let previous_1 = loan_read(d_1) { |guard_1|
  let projected_1 = ref_from_ptr(write, p_1, guard_1);
  replace(projected_1@prev, Option<ptr<Node>>::Some(p_3))
};
let previous_2 = loan_read(d_1) { |guard_2|
  let projected_2 = ref_from_ptr(write, p_1, guard_2);
  replace(projected_2@next, Option<ptr<Node>>::Some(p_2))
};
let previous_3 = loan_read(d_2) { |guard_3|
  let projected_3 = ref_from_ptr(write, p_2, guard_3);
  replace(projected_3@prev, Option<ptr<Node>>::Some(p_1))
};
let previous_4 = loan_read(d_2) { |guard_4|
  let projected_4 = ref_from_ptr(write, p_2, guard_4);
  replace(projected_4@next, Option<ptr<Node>>::Some(p_3))
};
let previous_5 = loan_read(d_3) { |guard_5|
  let projected_5 = ref_from_ptr(write, p_3, guard_5);
  replace(projected_5@prev, Option<ptr<Node>>::Some(p_2))
};
let vessel = Vessel { first: LiveRoot { p: p_0, a: a_0, d: d_0 }, second: LiveRoot { p: p_1, a: a_1, d: d_1 }, third: LiveRoot { p: p_2, a: a_2, d: d_2 }, fourth: LiveRoot { p: p_3, a: a_3, d: d_3 } };
let shipped = exchange(vessel);
let Vessel {first:x0, second:x1, third:x2, fourth:x3} = shipped;
let LiveRoot {p:pr0, a:al0, d:dm0} = x0;
let LiveRoot {p:pr1, a:al1, d:dm1} = x1;
let corrected_0=LiveRoot{p:pr0,a:al1,d:dm0};
let corrected_1=LiveRoot{p:pr1,a:al0,d:dm1};
retire(corrected_0);
retire(corrected_1);
retire(x2);
retire(x3);
let keeper_4=LiveRoot{p:p_4,a:a_4,d:d_4};
retire(keeper_4);
unit
},
}
},
}
},
}
},
}
},
}
}
