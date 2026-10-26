import gdb,json,hashlib,os
out=os.environ['P932_OUT']; records=[];plans=[];blocks={};errors=[];event=0
size_max=int(gdb.parse_and_eval('(size_t)-1'));width=size_max.bit_length()
def num(v):return int(v)
def addr(v):return hex(int(v))
def raw(s):
 d=s.dereference();return bytes(gdb.selected_inferior().read_memory(d['bytes'],int(d['length'])))
def sv(n):return n.dereference()['view']
def kind(n):return str(sv(n)['kind'])
def nxt(n):return n.dereference()['argument_next']
def word(src,span):return raw(src)[num(span['start_byte']):num(span['end_byte'])].decode()
def decimal(src,n,quantity):
 v=sv(n);assert str(v['kind'])==('NL_SYNTAX_USIZE_LITERAL' if quantity else 'NL_SYNTAX_U8_LITERAL')
 d=word(src,v['data']['usize_digits' if quantity else 'u8_digits']);assert d and all('0'<=c<='9' for c in d)
 value=0;maximum=size_max if quantity else 255
 for c in d:
  digit=ord(c)-48;assert value<=(maximum-digit)//10;value=value*10+digit
 return value

def derive(tree,src):
 assert tree.dereference()['source']==src
 u=sv(tree.dereference()['root']);assert str(u['kind'])=='NL_SYNTAX_FUNCTION_UNIT' and num(u['data']['function_unit']['count'])==2
 n=u['data']['function_unit']['declarations'];d=sv(n);assert str(d['kind'])=='NL_SYNTAX_AVS_STRUCT' and num(d['data']['avs_struct']['count'])==2
 name=word(src,d['data']['avs_struct']['name']);labels=[];f=d['data']['avs_struct']['fields']
 while int(f):
  v=sv(f);assert str(v['kind'])=='NL_SYNTAX_PARAMETER' and kind(v['data']['parameter']['type'])=='NL_SYNTAX_TYPE_NAME'
  assert word(src,sv(v['data']['parameter']['type'])['data']['name'])=='u8'
  labels.append(word(src,v['data']['parameter']['name']));f=nxt(f)
 assert len(labels)==2 and labels[0]!=labels[1]
 n=nxt(n);m=sv(n);assert str(m['kind'])=='NL_SYNTAX_FUNCTION' and not int(nxt(n)) and word(src,m['data']['function']['name'])=='main' and num(m['data']['function']['count'])==0
 body=m['data']['function']['body'];b=sv(body);assert str(b['kind'])=='NL_SYNTAX_BLOCK' and num(b['data']['block']['item_count'])==3
 result={'nominal':name,'labels':labels,'bindings':[],'initializer_order':[],'source_literals':[],'declared_values':[],'body_span':[num(b['span']['start_byte']),num(b['span']['end_byte'])],'binding_spans':[],'target_max':size_max,'target_value_bits':width}
 item=b['data']['block']['items']
 for i in range(2):
  v=sv(item);assert str(v['kind'])=='NL_SYNTAX_BINDING';result['bindings'].append(word(src,v['data']['binding']['name']));result['binding_spans'].append([num(v['span']['start_byte']),num(v['span']['end_byte'])])
  a=sv(v['data']['binding']['initializer']);assert str(a['kind'])=='NL_SYNTAX_AGGREGATE' and word(src,a['data']['aggregate']['type_name'])==name and num(a['data']['aggregate']['count'])==2
  f=a['data']['aggregate']['fields'];order=[];lits=[];values=[None,None]
  while int(f):
   q=sv(f);label=word(src,q['data']['binding']['name']);index=labels.index(label);assert index not in order
   literal=decimal(src,q['data']['binding']['initializer'],False);order.append(index);lits.append(literal);values[index]=literal;f=nxt(f)
  assert len(order)==2 and None not in values;result['initializer_order'].append(order);result['source_literals'].append(lits);result['declared_values'].append(values);item=nxt(item)
 assert result['bindings'][0]!=result['bindings'][1]
 statement=sv(item);assert str(statement['kind'])=='NL_SYNTAX_STATEMENT' and not int(nxt(item))
 choice=sv(statement['data']['statement']['expression']);assert str(choice['kind'])=='NL_SYNTAX_IF'
 c=sv(choice['data']['conditional']['condition']);op=str(c['kind']);assert op in ['NL_SYNTAX_USIZE_EQUALITY','NL_SYNTAX_USIZE_LESS_THAN']
 a=decimal(src,c['data']['equality']['left'],True);b=decimal(src,c['data']['equality']['right'],True)
 result['condition']={'operator':'<' if op.endswith('LESS_THAN') else '==','left':a,'right':b,'truth':a<b if op.endswith('LESS_THAN') else a==b}
 return result
class PlanDone(gdb.FinishBreakpoint):
 def __init__(self,source,span,result,tree):
  super().__init__(gdb.newest_frame(),internal=True);self.source=source;self.span=span;self.result=result;self.tree=tree
 def stop(self):
  try:
   assert num(self.return_value)==0
   body=self.result.dereference();core=derive(self.tree,self.source)
   assert core['body_span']==self.span and raw(self.source)[self.span[0]:self.span[1]]==raw(body.dereference()['source'])
   plans.append({'source':self.source,'tree':self.tree,'body':body,'core':core})
   records.append({'kind':'source_AST_before_snapshot','source_sha256':hashlib.sha256(raw(self.source)).hexdigest(),'core':core})
  except Exception as e:errors.append('plan:'+repr(e));return True
  return False
class Plan(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame();driver=f
  while driver and driver.name()!='compile_v0_to':driver=driver.older()
  assert driver;span=f.read_var('span')
  PlanDone(f.read_var('source'),[num(span['start_byte']),num(span['end_byte'])],f.read_var('out'),driver.read_var('unit'));return False
class BlockDone(gdb.FinishBreakpoint):
 def __init__(self,f,v,out):
  super().__init__(gdb.newest_frame(),internal=True);self.f=f;self.out=out
 def stop(self):
  global event
  if num(self.return_value)==0:
   event+=1;f=self.f.dereference();assert int(f['function_target'])
   blocks[num(self.f)]={'event':event,'block':num(self.out.dereference()),'function_target':addr(f['function_target']),'owner':addr(self.f)}
  return False
class Add(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame();v=f.read_var('view');fragment=f.read_var('fragment')
  if plans and str(v['kind'])=='NL_CHECKED_BLOCK' and fragment.dereference()['source']==plans[0]['body'].dereference()['source'] and num(v['span']['start_byte'])==0 and num(v['span']['end_byte'])==len(raw(fragment.dereference()['source'])):
   BlockDone(fragment,v,f.read_var('out'))
  return False
class Prepared(gdb.FinishBreakpoint):
 def __init__(self,f,c,node,definition):
  super().__init__(gdb.newest_frame(),internal=True);self.f=f;self.c=c;self.node=node;self.definition=definition
 def stop(self):
  try:
   assert num(self.return_value)==0 and len(plans)==1
   plan=plans[0];core=plan['core'];f=self.f.dereference();p=f['if_fields'].dereference();c=p['entry'].dereference();assert p['owner']==self.f and not int(p['arms'][0]) and not int(p['arms'][1])
   assert num(self.f) in blocks;block=blocks[num(self.f)]
   source=f['source'];assert source==plan['body'].dereference()['source']
   typ=[i+1 for i in range(num(c['type_count'])) if int(c['types'][i]['name']) and c['types'][i]['name'].string()==core['nominal']];assert len(typ)==1;typ=typ[0]
   fieldnames=[c['types'][typ-1]['field_names'][j].string() for j in range(2)];assert fieldnames==core['labels']
   rows=[]
   for i,name in enumerate(core['bindings']):
    bindings=[c['bindings'][j]['view'] for j in range(num(c['binding_count'])) if c['bindings'][j]['name'].string()==name];assert len(bindings)==1
    rid=num(bindings[0]['place']);root=c['places'][rid-1];package=c['values'][num(root['current_value'])-1];assert num(root['type'])==typ and num(package['owner_place'])==rid
    for j in range(2):
     cid=num(root['fixed_fields'][j]);child=c['places'][cid-1];v=c['values'][num(child['current_value'])-1]
     assert num(child['parent_aggregate'])==rid and num(v['aggregate_owner'])==num(root['current_value']) and str(v['carrier'])=='NL_CARRIER_AGGREGATE'
     assert num(package['fields'][j])==num(child['current_value']) and num(v['scalar_value'])==core['declared_values'][i][j]
     rows.append({'root_place':rid,'child_place':cid,'field':j,'scalar':num(v['scalar_value']),'raw_member_owner_VALUE':num(v['aggregate_owner']),'normalized_owner_PLACE':num(package['owner_place'])})
   choice=f['nodes'][self.node-1];cond=f['nodes'][num(choice['initializer'])-1];cv=c['values'][num(cond['results'][0]['value'])-1];assert str(cv['carrier'])=='NL_CARRIER_ENDED' and bool(cv['scalar_value'])==core['condition']['truth']
   first=f['nodes'][0]['first_item']
   for i in range(2):
    binding=f['nodes'][num(first)-1];a=f['nodes'][num(binding['initializer'])-1];assert num(a['type'])==typ
    assert num(binding['span']['start_byte'])+core['body_span'][0]==core['binding_spans'][i][0]
    field=a['first_argument'];order=[]
    for j in range(2):
     q=f['nodes'][num(field)-1];order.append(num(q['field_index']));literal=f['nodes'][num(q['initializer'])-1]
     assert num(literal['scalar_result']['value'])==core['source_literals'][i][j];field=q['next_argument']
    assert order==core['initializer_order'][i];first=binding['next_item']
   key={'unit_ast':addr(plan['tree']),'unit':addr(plan['source']),'plan':addr(plan['body']),'phase':'definition' if self.definition else 'actual_main','block_creation':block,'entry':addr(p['entry']),'check_world':addr(self.c),'graph_world':addr(f['context'])}
   records.append({'kind':'derived_core_to_actual_entry','key':key,'registered_nominal':typ,'rows':rows,'condition':bool(cv['scalar_value']),'new_discriminator':'raw owner ValueIds not substituted for normalized root PlaceIds; checked aggregate TypeId agrees with source nominal registry'})
   gdb.write('P932 derived '+json.dumps(records[-1])+'\n')
  except Exception as e:errors.append('prepare:'+repr(e));return True
  return False
class Prepare(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame();Prepared(f.read_var('f'),f.read_var('c'),num(f.read_var('id')),bool(f.older().read_var('check').dereference()['definition']));return False
class Destroy(gdb.Breakpoint):
 def stop(self):
  x=num(gdb.newest_frame().read_var('fragment'))
  if x in blocks:records.append({'kind':'block_owner_end','record':blocks.pop(x)})
  return False
class PlanEnd(gdb.Breakpoint):
 def stop(self):
  body=gdb.newest_frame().read_var('body')
  if plans and body==plans[0]['body'] and num(body.dereference()['owners'])==1:
   records.append({'kind':'registered_plan_end','body':addr(body)});plans.clear()
  return False
PlanEnd('nl_body_release',internal=True)
Plan('nl_body_create_span',internal=True);Add('nl_checked_add',internal=True);Prepare('nl_if_field_prepare',internal=True);Destroy('nl_checked_destroy',internal=True)
def end(e):open(out,'w').write(json.dumps({'exit':getattr(e,'exit_code',None),'records':records,'errors':errors,'remaining_blocks':len(blocks)},indent=2)+'\n')
gdb.events.exited.connect(end)
