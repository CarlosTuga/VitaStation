"""Convert the shipped PEM roots into DER byte arrays (stdlib only)."""
from pathlib import Path
import re,ssl,sys,json,hashlib
source,target=map(Path,sys.argv[1:])
blocks=re.findall(r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',source.read_text(encoding='utf-8'),re.S)
if not blocks:raise ValueError('No trust roots in bundled PEM')
certificates=[ssl.PEM_cert_to_DER_cert(block) for block in blocks]
if len(certificates)!=len(set(certificates)):raise ValueError('Duplicate trust roots')
# Bound X509 objects/locks on Vita. Selected public roots remain verified normally.
whitelist=json.loads((Path(__file__).parent/'tls_roots.json').read_text(encoding='utf-8'))
allowed={entry['sha256'] for entry in whitelist}
certificates=[der for der in certificates if hashlib.sha256(der).hexdigest() in allowed]
if len(certificates)!=len(allowed) or not certificates:raise ValueError('Pinned trust root set does not match bundled PEM')
blob=b''.join(certificates);offset=0;rows=[]
for cert in certificates:
 rows.append('{%d,%d}'%(offset,len(cert)));offset+=len(cert)
text='/* Generated from bundled Mozilla roots. No runtime CA file/PEM parsing. */\n#pragma once\n'
text+='static const unsigned char vs_ca_der[]={\n'+''.join(','.join('0x%02x'%b for b in blob[i:i+24])+',\n' for i in range(0,len(blob),24))+'};\n'
text+='static const struct {unsigned offset,length;} vs_ca_roots[]={'+','.join(rows)+'};\n'
target.write_text(text,encoding='ascii')
print('Embedded',len(certificates),'DER trust roots;',len(blob),'bytes')
