fn retune(field:ref<write,u8>,next:u8)->u8{let old=replace(field,next);return old;}
struct Controls { trim:u8, ceiling:u8 }
fn requested()->u8{u8(93)}
struct Packet { chain:Option<ptr<Packet>>, token:u8 }
fn main()->unit{
    let current=Controls{ceiling:u8(144),trim:u8(28)};
    let archive=Controls{trim:u8(2),ceiling:u8(177)};
    let old=loan_write(current@trim){|w|retune(w,requested())};
    let displaced=loan_write(archive@ceiling){|w|replace(w,old)};
    old;displaced;

match try_allocate_one<Packet>(){
 None=>{unit}, Some(bundle_a)=>{
 let OneBacking{allocation,raw}=bundle_a;let a_a=allocation;
let vacant_a=into_slot<Packet>(raw);let d_a=lifetime_domain();
let p_a=loan_read(d_a){|stable|initialize(vacant_a,Packet{chain:Option<ptr<Packet>>::None,token:u8(46)},stable)};
 match try_allocate_one<Packet>(){
  None=>{let ended_a=loan_exclusive_read(d_a){|end|destroy(p_a,end)};
let storage_a=erase_slot<Packet>(ended_a);
finalize_domain(d_a);deallocate(a_a,storage_a);unit},Some(bundle_b)=>{
   let OneBacking{allocation,raw}=bundle_b;let a_b=allocation;
let vacant_b=into_slot<Packet>(raw);let d_b=lifetime_domain();
let p_b=loan_read(d_b){|stable|initialize(vacant_b,Packet{chain:Option<ptr<Packet>>::None,token:u8(182)},stable)};
   let ended_b=loan_exclusive_read(d_b){|end|destroy(p_b,end)};
let storage_b=erase_slot<Packet>(ended_b);
finalize_domain(d_b);deallocate(a_b,storage_b);
   let ended_a=loan_exclusive_read(d_a){|end|destroy(p_a,end)};
let storage_a=erase_slot<Packet>(ended_a);
finalize_domain(d_a);deallocate(a_a,storage_a);unit
  },
 }
 },
}
}
