import os,pty,subprocess,select,time,sys
sys.path.insert(0,'.');from bridge import valid_state,valid_command,valid_source
state=b'PSTATE 14 500 500 500 333 650 260 250 150 400 0 0 48 0 0\n'
assert valid_state(state.strip())
assert not valid_state(state.replace(b' 14 ',b' 24 ').strip())
assert valid_command(b'MODEL 23') and not valid_command(b'MODEL 24')
assert valid_command(b'SET 8 1000') and not valid_command(b'SET 9 500')
assert not valid_command(b'BOOT') and not valid_command(b'UPDATE')
assert valid_source(b'SCENE 31 12 1 1 0 464952535420534F554E44')
for invalid in [b'SCENE SELECT 32',b'SCENE LOAD -1',b'SCENE SAVE 0 00',b'SCENE SAVE 0 2020',b'SCENE SAVE 0 GG',b'RESTART']:
 assert not valid_command(invalid),invalid
pod,ps=pty.openpty();display,ds=pty.openpty()
p=subprocess.Popen([sys.executable,'-u','bridge.py','--pod',os.ttyname(ps),'--display',os.ttyname(ds)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def receive(fd):
 data=b'';end=time.monotonic()+3
 while time.monotonic()<end:
  if select.select([fd],[],[],.1)[0]:data+=os.read(fd,4096)
  if b'\n' in data:return data
 raise AssertionError('No forwarded message')
try:
 time.sleep(.25);os.write(pod,state[:20]);os.write(pod,state[20:]);assert receive(display)==state
 for command in [b'MODEL 23\n',b'SET 8 250\n',b'MODE 1\n',b'PLAY 1\n',b'SCENE SELECT 31\n',b'SCENE LOAD 0\n',b'SCENE SAVE 0 464952535420534F554E44\n']:
  os.write(display,command);assert receive(pod)==command
 os.write(display,b'MODEL 24\nBOOT\n');assert not select.select([pod],[],[],.3)[0]
 metadata=b'SCENE 0 12 1 1 0 464952535420534F554E44\n';os.write(pod,metadata);assert receive(display)==metadata
 print('PASS: scenes metadata and controls; Plaits state relay, engine selection, parameter/mode/play controls, invalid-command rejection')
finally:
 p.terminate();p.communicate(timeout=3)
 for fd in (pod,ps,display,ds):os.close(fd)
