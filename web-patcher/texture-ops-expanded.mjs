import {makeGeneralTile,makePalletPage,makeBankRenderer} from './rom-tile-prototype.mjs';

const morton=(x,y)=>(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|((x&4)<<2)|((y&4)<<3);
const isInt=(v,min,max)=>Number.isSafeInteger(v)&&v>=min&&v<=max;

export function resolveTextures(desc, rom, revision, packSize) {
  if(!desc || !Array.isArray(desc.pages) || !Array.isArray(desc.operations)
      || desc.pages.length>512 || desc.operations.length>131072
      || (desc.banks && (!Array.isArray(desc.banks)||desc.banks.length>128))
      || !isInt(desc.building_offset,0,packSize)) throw Error('Invalid texture recipe');
  const writes=new Map(), generated=new Map(),renderers=new Map();
  const pallet=makePalletPage(rom,revision);
  for(const op of desc.operations) {
    const {page,x,y,id}=op;
    const layout=desc.pages[page];
    if(!isInt(page,0,desc.pages.length-1) || !Array.isArray(layout) || layout.length!==3)
      throw Error('Invalid texture page');
    const [w,h,start]=layout;
    if(!isInt(w,8,2048)||!isInt(h,8,2048)||w%8||h%8||!isInt(start,0,packSize-desc.building_offset-w*h*2)
        ||!isInt(x,0,w-8)||!isInt(y,0,h-8))throw Error('Texture operation out of bounds');
    const m=typeof id==='string'&&/^(general|pallet):([0-9]{1,3}):([0-3])$/.exec(id);
    const bank=typeof id==='string'&&/^bank:([0-9]{1,3}):([0-9]{1,4}):([0-3])$/.exec(id);
    if(!m&&!bank)throw Error('Invalid texture source');
    let tile=generated.get(id);
    if(!tile) {
      if(bank) {
        const index=Number(bank[1]),entry=desc.banks?.[index];
        if(!entry?.primary||!entry?.secondary)throw Error('Unknown retail texture bank');
        if(!renderers.has(index))renderers.set(index,makeBankRenderer(rom,revision,entry.primary,entry.secondary));
        const q=Number(bank[3]);
        tile=renderers.get(index)(Number(bank[2])).subarray(q*128,(q+1)*128);
      } else if(m[1]==='pallet') {
        const i=Number(m[2]),q=Number(m[3]);
        if(i>=89)throw Error('Invalid Pallet texture source');
        const px=(i%16)*2+q%2,py=Math.floor(i/16)*2+Math.floor(q/2);
        const offset=(py*32+px)*128;
        tile=pallet.subarray(offset,offset+128);
      } else {
        const i=Number(m[2]),q=Number(m[3]);
        if(i>=200)throw Error('Invalid General texture source');
        tile=makeGeneralTile(rom,revision,i).subarray(q*128,(q+1)*128);
      }
      generated.set(id,tile);
    }
    for(let dy=0;dy<8;dy++)for(let dx=0;dx<8;dx++) {
      const xx=x+dx,yy=y+dy;
      const offset=desc.building_offset+start+((yy>>3)*(w>>3)+(xx>>3))*128+2*morton(xx%8,yy%8);
      const j=2*morton(dx,dy), color=tile[j]|tile[j+1]<<8;
      if(writes.has(offset)&&writes.get(offset)!==color)throw Error('Conflicting texture operations');
      writes.set(offset,color);
    }
  }
  return writes;
}
