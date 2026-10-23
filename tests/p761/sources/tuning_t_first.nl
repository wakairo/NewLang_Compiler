struct ChannelTuning{bias:u8,ceiling:u8}
fn exchange(slot:ref<write,u8>,next:u8)->u8{let prior=replace(slot,next);return prior;}
struct LeaseCell{link:Option<ptr<LeaseCell>>,stamp:u8}
fn main()->unit{
let live=ChannelTuning{ceiling:u8(204),bias:u8(17)};
let saved=ChannelTuning{bias:u8(6),ceiling:u8(199)};
let prior=loan_write(live@ceiling){|field|exchange(field,u8(75))};
let displaced=loan_write(saved@bias){|field|exchange(field,prior)};
let current=live@ceiling;let sibling=live@bias;
prior;displaced;current;sibling;
match try_allocate_one<LeaseCell>(){
None=>{unit},Some(bundle_head)=>{
let OneBacking{allocation,raw}=bundle_head;let a_head=allocation;
let vacancy_head=into_slot<LeaseCell>(raw);let d_head=lifetime_domain();
let p_head=loan_read(d_head){|stable|initialize(vacancy_head,LeaseCell{link:Option<ptr<LeaseCell>>::None,stamp:u8(62)},stable)};
match try_allocate_one<LeaseCell>(){
None=>{let empty_head=loan_exclusive_read(d_head){|ending|destroy(p_head,ending)};
let bytes_head=erase_slot<LeaseCell>(empty_head);finalize_domain(d_head);deallocate(a_head,bytes_head);
unit},Some(bundle_tail)=>{
let OneBacking{allocation,raw}=bundle_tail;let a_tail=allocation;
let vacancy_tail=into_slot<LeaseCell>(raw);let d_tail=lifetime_domain();
let p_tail=loan_read(d_tail){|stable|initialize(vacancy_tail,LeaseCell{link:Option<ptr<LeaseCell>>::None,stamp:u8(187)},stable)};
let empty_tail=loan_exclusive_read(d_tail){|ending|destroy(p_tail,ending)};
let bytes_tail=erase_slot<LeaseCell>(empty_tail);finalize_domain(d_tail);deallocate(a_tail,bytes_tail);
let empty_head=loan_exclusive_read(d_head){|ending|destroy(p_head,ending)};
let bytes_head=erase_slot<LeaseCell>(empty_head);finalize_domain(d_head);deallocate(a_head,bytes_head);
unit
},
}
},
}
}
