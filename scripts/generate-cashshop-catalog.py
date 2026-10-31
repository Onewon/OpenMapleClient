"""Generate the Android cash-shop base catalogue from BeiDou v83 WZ XML."""
from pathlib import Path
import argparse
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,default=root.parent/'BeiDou-Server/gms-server/wz/Etc.wz/Commodity.img.xml')
p.add_argument('--output',type=Path,default=root/'app/src/main/cpp/src/Data/CashShopCatalog.inc')
a=p.parse_args()
rows=[]
for node in ET.parse(a.source).getroot():
    data={c.get('name'):c.get('value') for c in node}
    if 'SN' not in data:continue
    fields=[int(data.get(k,default)) for k,default in [('SN','0'),('ItemId','0'),('Count','1'),('Price','0'),('Period','0'),('Gender','2'),('OnSale','0')]]
    rows.append(fields)
assert rows and len({r[0] for r in rows})==len(rows)
a.output.write_text('// Generated from BeiDou v83 Commodity.img.xml; regenerate with scripts/generate-cashshop-catalog.py\n'+'\n'.join('{'+','.join(map(str,r))+'},' for r in rows)+'\n',encoding='utf8',newline='\n')
print('Generated',len(rows),'offers')