"""Exercise DER root import and a real libcurl SSL_CTX callback on desktop OpenSSL.
This is not a Vita runtime test. Uses Git for Windows' OpenSSL curl backend.
"""
import ctypes as C,os,re,ssl,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
binpath=Path('C:/Program Files/Git/mingw64/bin');dllpath=os.add_dll_directory(str(binpath))
crypto=C.CDLL(str(binpath/'libcrypto-3-x64.dll'));ssllib=C.CDLL(str(binpath/'libssl-3-x64.dll'));curl=C.CDLL(str(binpath/'libcurl-openssl-4.dll'))
crypto.X509_STORE_new.restype=C.c_void_p
crypto.d2i_X509.argtypes=[C.c_void_p,C.POINTER(C.c_void_p),C.c_long];crypto.d2i_X509.restype=C.c_void_p
crypto.X509_STORE_add_cert.argtypes=[C.c_void_p,C.c_void_p];crypto.X509_STORE_add_cert.restype=C.c_int
crypto.X509_free.argtypes=[C.c_void_p];crypto.X509_STORE_free.argtypes=[C.c_void_p]
ssllib.SSL_CTX_set_cert_store.argtypes=[C.c_void_p,C.c_void_p]
certs=[ssl.PEM_cert_to_DER_cert(x) for x in re.findall(r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',(root/'assets/cacert.pem').read_text(encoding='utf-8'),re.S)]
allowed={x['sha256'] for x in json.loads((root/'tools/tls_roots.json').read_text())}
certs=[x for x in certs if hashlib.sha256(x).hexdigest() in allowed]
assert len(certs)==len(allowed)
CALLBACK=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_void_p,C.c_void_p)
@CALLBACK
def install(handle,ctx,user):
 store=crypto.X509_STORE_new()
 if not store:return 27
 for der in certs:
  buffer=C.create_string_buffer(der);at=C.c_void_p(C.addressof(buffer));cert=crypto.d2i_X509(None,C.byref(at),len(der))
  if not cert:crypto.X509_STORE_free(store);return 58
  added=crypto.X509_STORE_add_cert(store,cert);crypto.X509_free(cert)
  if added!=1:crypto.X509_STORE_free(store);return 58
 ssllib.SSL_CTX_set_cert_store(ctx,store)
 print('SSL_CTX imported',len(certs),'DER trust roots')
 return 0
WRITE=C.CFUNCTYPE(C.c_size_t,C.c_void_p,C.c_size_t,C.c_size_t,C.c_void_p)
data=bytearray()
@WRITE
def write(ptr,size,count,user):
 n=size*count;data.extend(C.string_at(ptr,n));return n
curl.curl_global_init.argtypes=[C.c_long];assert curl.curl_global_init(3)==0
curl.curl_easy_init.restype=C.c_void_p
curl.curl_easy_setopt.argtypes=[C.c_void_p,C.c_int];curl.curl_easy_setopt.restype=C.c_int
curl.curl_easy_perform.argtypes=[C.c_void_p];curl.curl_easy_perform.restype=C.c_int
curl.curl_easy_cleanup.argtypes=[C.c_void_p]
urls=['https://raw.githubusercontent.com/Andiweli/HexFlow-Covers/main/Covers/PSVita/PCSG00706.png',json.loads((root/'assets/alive/catalog.json').read_bytes())[0]['icon']]
for url in urls:
 data.clear();h=curl.curl_easy_init();error=C.create_string_buffer(256);urlbuf=C.create_string_buffer(url.encode())
 for option,value in [(10002,C.cast(urlbuf,C.c_void_p)),(10065,C.c_void_p()),(10097,C.c_void_p()),(40309,C.c_void_p()),(321,C.c_long(0)),(20108,C.cast(install,C.c_void_p)),(64,C.c_long(1)),(81,C.c_long(2)),(20011,C.cast(write,C.c_void_p)),(10010,C.cast(error,C.c_void_p)),(52,C.c_long(1)),(13,C.c_long(30))]:
  result=curl.curl_easy_setopt(h,option,value);assert result==0,(option,result)
 result=curl.curl_easy_perform(h);curl.curl_easy_cleanup(h)
 assert result==0,(result,error.value)
 assert data.startswith(b'\x89PNG\r\n\x1a\n')
 print('TLS verified cover:',len(data),'bytes',url)
print('Desktop libcurl/OpenSSL callback test passed; Vita runtime pending.')
