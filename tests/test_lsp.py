"""Exercise the public LSP transport, including UTF-8 content lengths."""
from pathlib import Path
import json, subprocess, sys
from support import temporary_directory
EXE=Path(sys.argv[1]).resolve()
def packet(method,params=None,id=None):
    value={'jsonrpc':'2.0','method':method}
    if params is not None:value['params']=params
    if id is not None:value['id']=id
    body=json.dumps(value,ensure_ascii=False,separators=(',',':')).encode()
    return f'Content-Length: {len(body)}\r\n\r\n'.encode()+body
with temporary_directory('lsp-') as folder:
    path=Path(folder)/'馬.uma';uri=path.as_uri()
    requests=[packet('initialize',{},1),packet('initialized',{}),packet('textDocument/didOpen',{'textDocument':{'uri':uri,'languageId':'umascript','version':1,'text':'speed n="erro";'}}),packet('textDocument/didChange',{'textDocument':{'uri':uri,'version':2},'contentChanges':[{'text':'speed n=42;\nbakushin(n);\n'}]}),packet('textDocument/hover',{'textDocument':{'uri':uri},'position':{'line':0,'character':6}},2),packet('textDocument/completion',{'textDocument':{'uri':uri},'position':{'line':1,'character':0}},3),packet('textDocument/formatting',{'textDocument':{'uri':uri},'options':{'tabSize':4,'insertSpaces':True}},4),packet('textDocument/documentSymbol',{'textDocument':{'uri':uri}},5),packet('shutdown',{},6),packet('exit')]
    p=subprocess.run([str(EXE),'lsp'],input=b''.join(requests),capture_output=True,timeout=30)
    assert p.returncode==0,p.stderr
    data=p.stdout;messages=[]
    while data:
        header,body=data.split(b'\r\n\r\n',1);length=int(header.split(b':')[1].strip());messages.append(json.loads(body[:length]));data=body[length:]
    replies={m['id']:m for m in messages if 'id' in m}
    assert replies[1]['result']['capabilities']['textDocumentSync']==1
    reports=[m['params']['diagnostics'] for m in messages if m.get('method')=='textDocument/publishDiagnostics']
    assert reports[0][0]['code']=='U0002' and reports[1]==[],reports
    assert 'speed n' in replies[2]['result']['contents']['value']
    assert any(item['label']=='n' for item in replies[3]['result'])
    assert replies[4]['result'][0]['newText']=='speed n=42;\nbakushin(n);\n'
    assert any(item['name']=='n' for item in replies[5]['result'])
    assert replies[6]['result'] is None
print('LSP: initialize, diagnostics, edit, hover, completion, formatting, symbols and shutdown passed.')
