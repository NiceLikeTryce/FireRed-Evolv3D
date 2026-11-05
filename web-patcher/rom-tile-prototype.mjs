/* Owner-local experiment: generate voxel-ready metatile textures without a literal asset. */
const layouts = Object.freeze([0x2d4a94, 0x2d4b04]);
function gbaOffset(address, size) {
  const offset = address - 0x08000000;
  if (offset < 0 || offset >= size) throw Error('ROM pointer out of bounds');
  return offset;
}
function lz77(bytes, pos) {
  if (bytes[pos] !== 0x10 || pos + 4 > bytes.length) throw Error('Expected GBA LZ77 stream');
  const n = bytes[pos+1] | bytes[pos+2] << 8 | bytes[pos+3] << 16;
  if (!n || n > 640*32 || n%32) throw Error('Unexpected primary tile count');
  const out = new Uint8Array(n);
  let written=0;
  const byte=()=>{ if (pos>=bytes.length) throw Error('Truncated LZ77 stream'); return bytes[pos++]; };
  pos+=4;
  while (written<n) {
    const mask=byte();
    for(let bit=7;bit>=0 && written<n;bit--) {
      if (!(mask&(1<<bit))) out[written++]=byte();
      else {
        const a=byte(),b=byte(),len=(a>>4)+3,dist=((a&15)<<8|b)+1;
        if (dist>written || written+len>n) throw Error('Invalid GBA LZ77 copy');
        for(let i=0;i<len;i++,written++) out[written]=out[written-dist];
      }
    }
  }
  return out;
}
function banks(rom, revision, includePallet) {
  if (!(rom instanceof Uint8Array) || rom.length!==0x1000000 || ![0,1].includes(revision))
    throw Error('Expected a validated clean 16 MiB FireRed ROM and revision');
  const d=new DataView(rom.buffer,rom.byteOffset,rom.byteLength), t=layouts[revision];
  return [d,readBank(rom,d,t,1),includePallet?readBank(rom,d,t+24,0x101):null];
}
function readBank(rom,d,base,header) {
  if (!Number.isInteger(base)||base<0||base+24>rom.length||d.getUint32(base,true)!==header)
    throw Error('Unexpected tileset header');
  const offset=i=>gbaOffset(d.getUint32(base+i,true),rom.length);
  return {tiles:lz77(rom,offset(4)),pal:offset(8),mt:offset(12)};
}
export function makeBankRenderer(rom,revision,primary,secondary) {
  if (!(rom instanceof Uint8Array)||rom.length!==0x1000000||![0,1].includes(revision))
    throw Error('Expected a validated clean 16 MiB FireRed ROM and revision');
  const d=new DataView(rom.buffer,rom.byteOffset,rom.byteLength);
  const a=readBank(rom,d,primary.revisions[revision].header,1);
  const b=readBank(rom,d,secondary.revisions[revision].header,0x101);
  return (metatile)=>{
    if(!Number.isInteger(metatile)||metatile<0||
      (metatile<640?metatile>=primary.table_metatiles:metatile-640>=secondary.table_metatiles))
      throw Error('Metatile outside the selected tables');
    return render(d,a,b,metatile);
  };
}
function render(d, general, secondary, metatile) {
  const mt=metatile<640?general.mt+metatile*16:secondary.mt+(metatile-640)*16;
  const out=new Uint8Array(512), result=new DataView(out.buffer);
  for(let layer=0;layer<2;layer++) for(let quad=0;quad<4;quad++) {
    const word=d.getUint16(mt+(layer*4+quad)*2,true), id=word&1023, bank=word>>>12;
    const tileBank=id<640?general:secondary, paletteBank=bank<7?general:secondary;
    if (!tileBank || !paletteBank) throw Error('Prototype supports only General tiles and palettes');
    const tileOffset=(id<640?id:id-640)*32;
    if (tileOffset+32>tileBank.tiles.length) throw Error('Tile outside selected bank');
    for(let y=0;y<8;y++) for(let x=0;x<8;x++) {
      const sx=word&1024?7-x:x, sy=word&2048?7-y:y;
      const index=tileBank.tiles[tileOffset+sy*4+(sx>>1)]>>(sx%2*4)&15;
      if (!index) continue;
      const rgb=d.getUint16(paletteBank.pal+bank*32+index*2,true);
      const rgba=(rgb&31)<<11|((rgb>>5)&31)<<6|((rgb>>10)&31)<<1|1;
      const morton=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|((x&4)<<2)|((y&4)<<3);
      result.setUint16((quad*64+morton)*2,rgba,true);
    }
  }
  return out;
}
export function makeGeneralTile(rom, revision, metatile) {
  if (!Number.isInteger(metatile) || metatile<0 || metatile>=640) throw Error('Invalid General metatile');
  return render(...banks(rom,revision,false),metatile);
}
export function makePalletPage(rom,revision) {
  const source=banks(rom,revision,true);
  const page=new Uint8Array(256*128*2);
  for(let id=0;id<89;id++) {
    const tile=render(...source,640+id);
    for(let quad=0;quad<4;quad++) {
      const x=(id%16)*2+(quad%2),y=Math.floor(id/16)*2+Math.floor(quad/2);
      page.set(tile.subarray(quad*128,(quad+1)*128),(y*32+x)*128);
    }
  }
  return page;
}
