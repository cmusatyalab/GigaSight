#!/usr/bin/env python
from collections import deque
import asyncore
import socket
import tempfile
import struct
import time
import logging
from threading import Thread
import Queue

OutputDirs = deque()
#OutputDirs.extend(("/tmp/test0-", "/mnt/tmp/test1-" ))
OutputDirs.extend(("/tmp/test0-", "/tmp/test1-" ))
Port = 8080
ControlPort = 8081
WRITE_SIZE = 65536
#LOG_LEVEL=logging.DEBUG
LOG_LEVEL=logging.INFO
LOG_FORMAT = "%(asctime)-15s %(levelname)s %(message)s"
LOG_FILE = "/tmp/cloud_server.log"
PERF_LOG_INTERVAL = 1
NUM_WRITERS = 1

logging.basicConfig(format=LOG_FORMAT,filename=LOG_FILE,level=LOG_LEVEL)

writeQ = Queue.Queue()

def writer():
    while True:
	f, s = writeQ.get()
	if s==None:
	    f.close()
	else:
	    f.write(s)
	    PerfLog.add_wrote( len(s) )
	writeQ.task_done()

def queue_write( f, s ):
	writeQ.put( ( f, s ) )

def queue_close( f ):
	writeQ.put( ( f, None ) )

class PerfLog:
    recv = 0
    last_recv = 0
    wrote = 0
    last_wrote = 0
    last_time = time.time()
    q = Queue.Queue()

    @staticmethod
    def do_log():
	tm = time.time()
        delta = tm-PerfLog.last_time
	if delta > PERF_LOG_INTERVAL:
	    r = (PerfLog.recv-PerfLog.last_recv)/delta/1000000
	    w = (PerfLog.wrote-PerfLog.last_wrote)/delta/1000000
	    logging.info("Perf: recv %.3f MBps, wrote %.3f MBps" % (r, w) )
	    PerfLog.last_wrote = PerfLog.wrote
	    PerfLog.last_recv = PerfLog.recv
	    PerfLog.last_time = tm

    @staticmethod
    def do_work():
	while True:
	    t, v = PerfLog.q.get()
	    if t=='r':
	        PerfLog.recv += v
	    else:
	        PerfLog.wrote += v
	    PerfLog.do_log()
	    PerfLog.q.task_done()

    @staticmethod
    def add_recv(x):
	PerfLog.q.put( ('r', x) )

    @staticmethod
    def add_wrote(x):
	PerfLog.q.put( ('w', x) )

    @staticmethod
    def start():
	t= Thread( target=PerfLog.do_work )
	t.setDaemon(True)
	t.start()


class ClientQueue(deque):
    def append(self,x):
        h = None
	if len(self)>0:
	    h = self[0]
	deque.append(self, x)
	if ( len(self)>0 and h!=self[0] ):
	    self[0].handle_head_of_queue()

    def appendleft(self,x):
    	deque.appendleft(self,x)
	if len(self[0])>0:
	    self[0].handle_head_of_queue()

    def extend(self,x):
        h = None
	if len(self)>0:
	    h = self[0]
	deque.extend(self, x)
	if ( len(self)>0 and h!=self[0] ):
	    self[0].handle_head_of_queue()

    def extendleft(self,x):
    	deque.extendleft(self,x)
	if len(self)>0:
	    self[0].handle_head_of_queue()

    def popleft(self):
    	v = deque.popleft(self)
	if len(self[0])>0:
	    self[0].handle_head_of_queue()
	return v

    def remove(self,x):
        h = None
	if len(self)>0:
	    h = self[0]
	deque.remove(self, x)
	if ( len(self)>0 and h!=self[0] ):
	    self[0].handle_head_of_queue()

    def reverse(self):
    	deque.reverse(self)
	if len(self)>0:
	    self[0].handle_head_of_queue()

    def rotate(self,x):
    	deque.rotate(self,x)
	if len(self)>0:
	    self[0].handle_head_of_queue()

    def rotate_out(self, x):
    	self.remove(x)
	self.append(x)


class ClientHandler(asyncore.dispatcher_with_send):
    
    CQ = ClientQueue()
    Enable = False;
    
    def __init__(self,sock,addr):
        asyncore.dispatcher_with_send.__init__(self,sock)
        #self.out_file = tempfile.NamedTemporaryFile( prefix=OutputDirs[0], delete=True )
        self.out_file = tempfile.NamedTemporaryFile( prefix=OutputDirs[0], delete=False )
        OutputDirs.rotate()
	self.wait_for_length = True
	self.data_length = 0
	self.data_remain = 0
	self.data_read = 0
	self.data = ""
	self.start_read_time = 0
	self.last_start_time = time.time()
	self.addrstr = str(addr)
	ClientHandler.CQ.append( self )

    def __str__(self):
	return self.addrstr

    def handle_read(self):
	if self.wait_for_length:
	    logging.debug("Reading length")
	    d = self.recv(4)
	    l = 0;
	    if len(d)==4:
		l = (struct.unpack('!i', d ))[0]
		self.start_read_time = time.time()
	    else:
		logging.debug("CLOSE in read")
		self.do_close()
		return
	    self.data_length = l
	    self.data_remain = l
	    self.data_read = 0
	    self.wait_for_length = False
	    self.data = ""
	    logging.debug("Expecting "+str(l)+" bytes from "+self.addrstr)
	else:
	    l = self.data_remain - self.data_read;
	    if l>1.5*WRITE_SIZE:
		l = WRITE_SIZE
	    logging.debug("Reading data")
	    tmpdata = self.recv( l )
	    if len(tmpdata)==0:
		logging.debug("CLOSE in read")
		self.do_close()
		return
	    self.data = self.data + tmpdata
	    self.data_read = self.data_read + len(tmpdata)
	    logging.debug("Recieved "+str(len(tmpdata))+" from "+self.addrstr)
	    PerfLog.add_recv(len(tmpdata))
	
	if (self.data_read > WRITE_SIZE and self.data_remain>1.5*WRITE_SIZE):
	    queue_write( self.out_file, self.data[:WRITE_SIZE-1] )
	    self.data_read-=WRITE_SIZE
	    self.data_remain-=WRITE_SIZE
	    self.data = self.data[WRITE_SIZE:]
	    logging.debug("Wrote "+str(WRITE_SIZE)+" bytes to "+self.out_file.name)
	if (self.data_read >= self.data_remain):
	    queue_write( self.out_file, self.data )
	    logging.debug("Wrote "+str(len(self.data))+" bytes to "+self.out_file.name)
	    tm = time.time()
	    logging.info( "Client xfer (id, size, time, interval): %s, %.02f MB, %.2f ms, %.2f s" % (self.addrstr, self.data_length/1000000, 1000*(tm-self.start_read_time), self.start_read_time-self.last_start_time) )
	    self.data_read=0
	    self.data=""
	    self.wait_for_length=True
	    self.last_start_time = self.start_read_time
	    ClientHandler.CQ.rotate_out(self)

    def handle_close(self):
	logging.info( 'close called '+str(self) )

    def do_close(self):
	queue_close( self.out_file )
	if self in ClientHandler.CQ:
	    ClientHandler.CQ.remove(self)
	self.close()

    def handle_head_of_queue(self):
	if ClientHandler.Enable:
	    logging.debug( 'sent ok to transmit to '+str(self) )
            self.send( '\001' )

    @staticmethod
    def disable():
	ClientHandler.Enable = False

    @staticmethod
    def enable():
	ClientHandler.Enable = True
	if len(ClientHandler.CQ) > 0:
	   ClientHandler.CQ[0].handle_head_of_queue()


class MyServer(asyncore.dispatcher):

    def __init__(self, port):
	asyncore.dispatcher.__init__(self)
	self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
	self.set_reuse_addr()
	#self.bind(('localhost', port))
	self.bind(('0.0.0.0', port))
	self.listen(5)
	logging.info("Main server started on port "+str(port))

    def handle_accept(self):
	pair = self.accept()
	if pair is None:
	    pass
	else:
	    sock, addr = pair
	    sock.setsockopt( socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
	    logging.info( 'Connection from %s' % repr(addr) )
	    handler = ClientHandler(sock, addr)


class ControlHandler(asyncore.dispatcher_with_send):
    
    def __init__(self,sock,addr):
        asyncore.dispatcher_with_send.__init__(self,sock)
	self.addrstr = str(addr)
	self.comment =""
	self.is_reading_comment = False

    def __str__(self):
	return self.addrstr

    def handle_read(self):
	c = self.recv(1)
	if self.is_reading_comment:
	    if c == '\n':
		logging.info( "Control Channel: Comment: "+self.comment )
		self.comment = ""
		self.is_reading_comment = False
	    else:
		self.comment = self.comment+c
	else:
	    if c == 'Q':
		logging.info( "Control Channel: Received Quit" )
		logging.shutdown()
		exit()
	    if c == 'S':
		logging.info( "Control Channel: Received Start" )
		ClientHandler.enable()
		ClientHandler.disable()
	    if c == 'X':
		logging.info( "Control Channel: Received Stop" )
		ClientHandler.disable()
	    if c == 'C':
		self.is_reading_comment = True

    def handle_close(self):
	logging.info( 'control channel close called '+str(self) )
	self.close()


class ControlServer(asyncore.dispatcher):

    def __init__(self, port):
	asyncore.dispatcher.__init__(self)
	self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
	self.set_reuse_addr()
	#self.bind(('localhost', port))
	self.bind(('0.0.0.0', port))
	self.listen(5)
	logging.info("Control server started on port "+str(port))

    def handle_accept(self):
	pair = self.accept()
	if pair is None:
	    pass
	else:
	    sock, addr = pair
	    sock.setsockopt( socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
	    logging.info( 'Control connection from %s' % repr(addr) )
	    handler = ControlHandler(sock, addr)


server = MyServer( Port )
controller = ControlServer( ControlPort )
PerfLog.start()
for i in range(NUM_WRITERS):
    t = Thread( target=writer )
    t.setDaemon(True)
    t.start()
asyncore.loop()
logging.info("Server exit")

