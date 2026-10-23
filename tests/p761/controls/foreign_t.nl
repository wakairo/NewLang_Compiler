struct Segment{next:Option<ptr<Segment>>,opcode:u8}
fn exchange(slot:ref<write,u8>,next:u8)->u8{let prior=replace(slot,next);return prior;}
struct ForeignLimits{floor:u8,cap:u8}
fn main()->unit{
let live=ForeignLimits{cap:u8(168),floor:u8(32)};
let saved=ForeignLimits{floor:u8(4),cap:u8(219)};
let prior=loan_write(live@floor){|field|exchange(field,u8(91))};
let displaced=loan_write(saved@cap){|field|replace(field,prior)};
let current=live@floor;let sibling=live@cap;
prior;displaced;current;sibling;
match try_allocate_one<Segment>(){
None=>{unit},Some(bundle_head)=>{
let OneBacking{allocation,raw}=bundle_head;let a_head=allocation;
let vacancy_head=into_slot<Segment>(raw);let d_head=lifetime_domain();
let p_head=loan_read(d_head){|stable|initialize(vacancy_head,Segment{next:Option<ptr<Segment>>::None,opcode:u8(57)},stable)};
match try_allocate_one<Segment>(){
None=>{let empty_head=loan_exclusive_read(d_head){|ending|destroy(p_head,ending)};
let bytes_head=erase_slot<Segment>(empty_head);finalize_domain(d_head);deallocate(a_head,bytes_head);
unit},Some(bundle_tail)=>{
let OneBacking{allocation,raw}=bundle_tail;let a_tail=allocation;
let vacancy_tail=into_slot<Segment>(raw);let d_tail=lifetime_domain();
let p_tail=loan_read(d_tail){|stable|initialize(vacancy_tail,Segment{next:Option<ptr<Segment>>::None,opcode:u8(143)},stable)};
let empty_tail=loan_exclusive_read(d_tail){|ending|destroy(p_tail,ending)};
let bytes_tail=erase_slot<Segment>(empty_tail);finalize_domain(d_tail);deallocate(a_tail,bytes_tail);
let empty_head=loan_exclusive_read(d_head){|ending|destroy(p_head,ending)};
let bytes_head=erase_slot<Segment>(empty_head);finalize_domain(d_head);deallocate(a_head,bytes_head);
unit
},
}
},
}
}
