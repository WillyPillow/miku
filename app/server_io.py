#!/usr/bin/env python3

from tioj_url import tioj_url, tioj_key
from urllib.request import urlopen, urlretrieve, URLError, ContentTooShortError
from urllib.parse import urlencode
from datetime import datetime
import sys, inspect, os

# open stdin / stdout as binary
stdin = open(sys.stdin.fileno(), 'rb', closefd = False)
stdout = open(sys.stdout.fileno(), 'wb', buffering = 0, closefd = False)

def Log(*args, **kwargs):
    print(datetime.now().strftime('%Y-%m-%d %H:%M:%S %f')[:-3], *args, file = sys.stderr, flush = True, **kwargs)
def ErrExit(msg):
    Log('[server_io.py] Error: ' + msg)
    exit(1)
def URL(item, **kwargs):
    ret = tioj_url + '/fetch/' + item + '?key=' + tioj_key
    for key, val in kwargs.items(): ret += '&' + key + '=' + val
    return ret
def Read():
    header = stdin.read(8)
    if len(header) != 8: ErrExit('Got EOF')
    query_type = header[0]
    no_rep = header[1]
    query_id = int.from_bytes(header[2:4], byteorder = 'little')
    size = int.from_bytes(header[4:8], byteorder = 'little')
    if query_type == 8:
        msg = stdin.read(size).split(b'\0', 1)
        return (query_type, no_rep, query_id, [msg[0].decode('utf-8'), msg[1]])
    return (query_type, no_rep, query_id, [i.decode('utf-8') for i in stdin.read(size).split(b'\0')])
def Write(query_id, msg, status = 0):
    stdout.write(query_id.to_bytes(2, byteorder = 'little') + \
            bytes([status, 0]) + len(msg).to_bytes(4, byteorder = 'little') + msg)

def FetchCode(sub_id):
    return urlopen(URL('code', sid = sub_id)).read()
def FetchInterlib(problem_id):
    return urlopen(URL('interlib', pid = problem_id)).read()
def FetchLimits(problem_id):
    return urlopen(URL('testdata_limit', pid = problem_id)).read()
def FetchSjCode(problem_id):
    return urlopen(URL('sjcode', pid = problem_id)).read()
def FetchSubmission():
    return urlopen(URL('submission')).read()
def FetchTestdata(tid, problem_id, testdata_id):
    problem_id = problem_id.zfill(4)
    testdata_id = testdata_id.zfill(3)
    try: os.makedirs('./testdata/{0}'.format(problem_id))
    except: pass
    path = './testdata/{0}/{{}}{1}'.format(problem_id, testdata_id)
    urlretrieve(URL('testdata', input = '', tid = tid), path.format('input'))
    urlretrieve(URL('testdata', tid = tid), path.format('output'))
    return b''
def FetchTestdataMeta(problem_id):
    return urlopen(URL('testdata_meta', pid = problem_id)).read()
def RespondValidating(submission_id):
    urlopen(URL('validating', sid = submission_id))
    return b''
def UpdateMessage(submission_id, msg):
    msg_enc = urlencode({"message": msg}).encode('utf-8')
    urlopen(URL('write_message', sid = submission_id), data = msg_enc)
    return b''
def UpdateVerdict(submission_id, verdict, status):
    urlopen(URL('write_result', sid = submission_id, result = verdict, \
            status = status))
    return b''

mapping = {
    0: FetchCode,
    1: FetchInterlib,
    2: FetchLimits,
    3: FetchSjCode,
    4: FetchSubmission,
    5: FetchTestdata,
    6: FetchTestdataMeta,
    7: RespondValidating,
    8: UpdateMessage,
    9: UpdateVerdict
}

# Alive check
if Read() == (255, 0, 0, ['']): Write(0, b'')
else: ErrExit('Alive check failed')

while True:
    query_type, no_rep, query_id, msg = Read()
    if msg == ['']: msg = []
    if query_type not in mapping:
        if not no_rep: Write(query_id, b'', 255)
        continue
    func = mapping[query_type]
    if len(inspect.getargspec(func).args) != len(msg):
        if not no_rep: Write(query_id, b'', 255)
        continue
    try:
        msg = mapping[query_type](*msg)
        if not no_rep: Write(query_id, msg)
    except URLError as e:
        if not no_rep: Write(query_id, str(e.reason).encode('utf-8'), 1)
    except ContentTooShortError as e:
        if not no_rep: Write(query_id, b'', 2)
    except:
        if not no_rep: Write(query_id, b'', 3)
