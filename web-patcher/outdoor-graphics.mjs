// Deterministic GBA LZ77 encoding. The match order follows gbagfx.
// Copyright (c) 2015 YamaArashi. See GBAGFX-LICENSE.txt.
export function decodeLz77(source, offset, maximumSize = 20480) {
  if (!Number.isSafeInteger(offset) || offset < 0 || offset + 4 > source.length
      || source[offset] !== 0x10) throw Error('Invalid outdoor tile source.');
  const size = source[offset + 1] | source[offset + 2] << 8 | source[offset + 3] << 16;
  if (!Number.isSafeInteger(maximumSize) || maximumSize < 1 || maximumSize > 65536
      || size < 1 || size > maximumSize) throw Error('Unexpected outdoor tile size.');
  const output = new Uint8Array(size);
  let cursor = offset + 4;
  let written = 0;
  const byte = () => {
    if (cursor >= source.length) throw Error('Truncated outdoor tile data.');
    return source[cursor++];
  };
  while (written < size) {
    const flags = byte();
    for (let bit = 7; bit >= 0 && written < size; bit--) {
      if (!(flags & (1 << bit))) {
        output[written++] = byte();
        continue;
      }
      const first = byte();
      const distance = ((first & 15) << 8 | byte()) + 1;
      const length = (first >> 4) + 3;
      if (distance > written || written + length > size) throw Error('Invalid outdoor tile copy.');
      for (let i = 0; i < length; i++, written++) output[written] = output[written - distance];
    }
  }
  return output;
}

export function decodeTiles(source, offset) {
  const output = decodeLz77(source, offset);
  if (output.length !== 20480) throw Error('Unexpected outdoor tile size.');
  return output;
}

export function compressTiles(source) {
  if (source.length !== 20480) throw Error('Unexpected outdoor tile size.');
  const output = new Uint8Array(4 + source.length + Math.ceil(source.length / 8) + 3);
  output.set([0x10, source.length & 255, source.length >> 8, source.length >> 16]);
  const previous = new Int32Array(source.length).fill(-1);
  const heads = new Map();
  const key = at => source[at] << 16 | source[at + 1] << 8 | source[at + 2];
  let indexed = 0;
  let position = 0;
  let written = 4;
  while (position < source.length) {
    const flags = written++;
    for (let bit = 7; bit >= 0 && position < source.length; bit--) {
      while (indexed < position && indexed + 2 < source.length) {
        const code = key(indexed);
        previous[indexed] = heads.get(code) ?? -1;
        heads.set(code, indexed++);
      }
      let length = 0;
      let distance = 0;
      if (position + 2 < source.length) {
        for (let candidate = heads.get(key(position)) ?? -1;
          candidate >= 0 && position - candidate <= 4096; candidate = previous[candidate]) {
          if (position - candidate < 2) continue;
          let count = 0;
          while (count < 18 && position + count < source.length
            && source[candidate + count] === source[position + count]) count++;
          if (count > length) {
            length = count;
            distance = position - candidate;
            if (count === 18) break;
          }
        }
      }
      if (length >= 3) {
        output[flags] |= 1 << bit;
        output[written++] = (length - 3) << 4 | (distance - 1) >> 8;
        output[written++] = (distance - 1) & 255;
        position += length;
      } else output[written++] = source[position++];
    }
  }
  return output.slice(0, Math.ceil(written / 4) * 4);
}

export function rebuildOutdoorBank(recipe, rom, revision, pool) {
  if (!recipe || ![0, 1].includes(revision) || !Array.isArray(recipe.sources)
      || recipe.sources.length !== 2 || !Array.isArray(recipe.edits)
      || recipe.edits.length > 640 || !(pool instanceof Uint8Array) || pool.length % 32) {
    throw Error('Invalid outdoor graphics recipe.');
  }
  const tiles = decodeTiles(rom, recipe.sources[revision]);
  const used = new Set();
  for (const [tile, from] of recipe.edits) {
    if (!Number.isInteger(tile) || tile < 0 || tile >= 640 || used.has(tile)
        || !Number.isInteger(from) || from < 0 || from >= pool.length / 32) {
      throw Error('Outdoor graphics edit is out of bounds.');
    }
    used.add(tile);
    tiles.set(pool.subarray(from * 32, from * 32 + 32), tile * 32);
  }
  return compressTiles(tiles);
}
