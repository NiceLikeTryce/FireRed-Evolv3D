import {resolveTextures} from './texture-ops-expanded.mjs';
import {rebuildShortFont} from './rebuild-short-font.mjs';
import {rebuildOutdoorBank} from './outdoor-graphics.mjs';
import {applyApprovedGamePatch, applyPatchedGameReferences} from './approved-game-source.mjs';
export const ROM_SIZE = 16 * 1024 * 1024;
const files = ['launcher.gz', 'romfs-recipe.gz', 'romfs-literals.gz', 'romfs-textures.gz'];
const launcherRecipeName = 'launcher-recipe.gz';
const gameRecipeName = 'launcher-game-recipe.gz';
const gamePatchNames = ['FireRed_Evolv3D_assets_v2_from_BPRE_USA.bps.gz', 'FireRed_Evolv3D_assets_v2_from_BPRE_USA_Rev1.bps.gz'];
const rowSize = 16;
const hex = /^[0-9a-f]{64}$/;
const version = /^v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)$/;
const maxOutput = 192 * 1024 * 1024;
const integer = (n, min, max) => Number.isSafeInteger(n) && n >= min && n <= max;

export async function sha256(bytes) {
  const hash = await crypto.subtle.digest('SHA-256', bytes);
  return Array.from(new Uint8Array(hash), b => b.toString(16).padStart(2, '0')).join('');
}

export function validateManifest(m) {
  if (m?.schema === 11) {
    if (m.base_build?.schema !== 10 || m.base_build.distribution_scope !== 'public-patch-data') throw Error('Invalid base build information.');
    const base = validateManifest(m.base_build);
    const updateName = 'runtime-update.bps.gz';
    const update = m.parts?.[updateName];
    if (!version.test(m.version) || m.distribution_scope !== 'public-patch-data'
        || !Array.isArray(m.sources) || JSON.stringify(m.sources) !== JSON.stringify(base.sources)
        || !m.parts || Object.keys(m.parts).length !== Object.keys(base.parts).length + 1
        || Object.keys(base.parts).some(name => JSON.stringify(m.parts[name]) !== JSON.stringify(base.parts[name]))
        || !integer(update?.size, 1, maxOutput) || !integer(update?.decoded_size, 16, maxOutput)
        || !hex.test(update?.sha256) || !hex.test(m.runtime_patch_sha256)
        || !integer(m.output?.size, 44, maxOutput) || !hex.test(m.output.sha256)) {
      throw Error('Invalid runtime update information.');
    }
    return m;
  }
  const isLocalCandidate = typeof m?.version === 'string' && /^beta-rc[0-9]+$/.test(m.version)
    && m.distribution_scope === 'owner-local';
  const isPublishedPatch = version.test(m?.version) && m?.distribution_scope === 'public-patch-data'
    && typeof m.runtime_build === 'string' && /^beta-rc[0-9]+$/.test(m.runtime_build);
  const recipeLauncher = [4, 5, 6, 7, 10].includes(m?.schema);
  const components = recipeLauncher ? [...files, launcherRecipeName] : files;
  if ([6,7].includes(m?.schema)) components.push('outdoor-tiles.gz');
  if (m?.schema === 10) components.push(gameRecipeName, ...gamePatchNames);
  const prefixSize = recipeLauncher ? m.launcher_size : m?.parts?.['launcher.gz']?.decoded_size;
  if (![3,4,5,6,7,10].includes(m?.schema) || !(isLocalCandidate || isPublishedPatch)
      || !Array.isArray(m.sources) || m.sources.length !== 2
      || m.sources.map(s => s.revision).join(',') !== 'v1.0,v1.1'
      || !m.sources.every(s => hex.test(s.sha256))
      || m.sources[0].sha256 === m.sources[1].sha256
      || !components.every(name => integer(m.parts?.[name]?.size, 1, maxOutput)
        && integer(m.parts[name].decoded_size, 1, maxOutput) && hex.test(m.parts[name].sha256))
      || !integer(m.romfs?.size, 1, maxOutput) || !hex.test(m.romfs.sha256)
      || !integer(m.romfs.literal_size, 1, m.romfs.size)
      || !integer(m.output?.size, 44, maxOutput) || !hex.test(m.output.sha256)
      || !integer(prefixSize, 44, maxOutput)
      || prefixSize + m.romfs.size !== m.output.size
      || (recipeLauncher && (m.parts['launcher.gz'].decoded_size >= prefixSize
        || m.parts[launcherRecipeName].decoded_size > 1024 * 1024
        || m.parts[launcherRecipeName].decoded_size % rowSize !== 0))
      || m.parts['romfs-literals.gz'].decoded_size !== m.romfs.literal_size
      || m.parts['romfs-textures.gz'].decoded_size > (m.schema >= 7 ? 8 : 2) * 1024 * 1024
      || m.parts['romfs-recipe.gz'].decoded_size % rowSize !== 0) {
    throw new Error('Invalid 3DS build information.');
  }
  if (m.schema === 10 && (!m.game_patch || !version.test(m.game_patch.version)
      || !integer(m.game_patch.target_size, 1, ROM_SIZE) || !hex.test(m.game_patch.target_sha256)
      || m.game_patch.kind !== 'approved-gba-assets-v2' || !hex.test(m.game_patch.source_game_sha256)
      || !integer(m.game_patch.pool_size, 1, 4096) || m.game_patch.pool_size % 32
      || !integer(m.game_patch.pool_offset, 0, m.game_patch.target_size - m.game_patch.pool_size)
      || !integer(m.game_reference_bytes, 1, prefixSize)
      || m.parts[gameRecipeName].decoded_size > 1024 * 1024
      || m.parts[gameRecipeName].decoded_size % 12 !== 0
      || !Array.isArray(m.game_patch.patch_sha256) || m.game_patch.patch_sha256.length !== 2
      || !m.game_patch.patch_sha256.every(value => hex.test(value))
      || gamePatchNames.some(name => m.parts[name].size > 32 * 1024 * 1024
        || m.parts[name].decoded_size > 32 * 1024 * 1024))) {
    throw Error('Invalid approved game patch information.');
  }
  return m;
}

async function fillGaps(compressed, size, destination, gaps) {
  let gap = 0;
  let position = gaps[0][0];
  await inflate(compressed, size, chunk => {
    let consumed = 0;
    while (consumed < chunk.length) {
      const count = Math.min(gaps[gap][1] - position, chunk.length - consumed);
      destination.set(chunk.subarray(consumed, consumed + count), position);
      consumed += count;
      position += count;
      if (position === gaps[gap][1] && ++gap < gaps.length) position = gaps[gap][0];
    }
  });
  if (gap !== gaps.length) throw new Error('Build literal data is incomplete.');
}

export async function detectROM(bytes, manifest) {
  if (bytes.byteLength !== ROM_SIZE) throw new Error('Select a clean 16 MiB US/English FireRed ROM.');
  const hash = await sha256(bytes);
  const index = manifest.sources.findIndex(source => source.sha256 === hash);
  if (index < 0) throw new Error('This ROM does not match clean US/English FireRed v1.0 or v1.1.');
  return index;
}

async function inflate(bytes, expectedSize, write) {
  const stream = new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'));
  const reader = stream.getReader();
  let position = 0;
  try {
    while (true) {
      const {done, value} = await reader.read();
      if (done) break;
      if (value.length > expectedSize - position) throw new Error('Build component is larger than declared.');
      write(value, position);
      position += value.length;
    }
  } finally {
    await reader.cancel();
  }
  if (position !== expectedSize) throw new Error('Build component is incomplete.');
}

function applyReferences(recipe, source, revision, pack, expectedLiterals, sorted) {
  const table = new DataView(recipe.buffer, recipe.byteOffset, recipe.byteLength);
  const gaps = [];
  let position = 0;
  let literalSize = 0;
  function addGap(end) {
    if (end === position) return;
    gaps.push([position, end]);
    literalSize += end - position;
  }
  let pixel=0;
  const address=n=>Math.floor(n/65536);
  function textureThrough(limit) {
    while(pixel<sorted.length&&address(sorted[pixel])<limit){
      const packed=sorted[pixel++],offset=address(packed),value=packed&65535;
      if(offset<position||offset+2>limit)throw Error('Texture recipe overlaps another component');
      addGap(offset);
      pack[offset]=value&255;
      pack[offset+1]=value>>>8;
      position=offset+2;
    }
  }
  for (let row = 0; row < table.byteLength; row += rowSize) {
    const offset = table.getUint32(row, true);
    const length = table.getUint32(row + 4, true);
    const v10 = table.getUint32(row + 8, true);
    const v11 = table.getUint32(row + 12, true);
    if (!integer(offset, position, pack.length) || !integer(length, 1, pack.length - offset)
        || !integer(v10, 0, source.length - length) || !integer(v11, 0, source.length - length)) {
      throw new Error('RomFS reference is out of bounds.');
    }
    textureThrough(offset);
    addGap(offset);
    const from = revision === 0 ? v10 : v11;
    pack.set(source.subarray(from, from + length), offset);
    position = offset + length;
    while(pixel<sorted.length&&address(sorted[pixel])<position) {
      const packed=sorted[pixel++],at=address(packed),value=packed&65535;
      if(at<offset||at+2>position||pack[at]!== (value&255)||pack[at+1]!== (value>>>8))
        throw Error('Texture recipe conflicts with a direct ROM reference');
    }
  }
  textureThrough(pack.length);
  addGap(pack.length);
  if (literalSize !== expectedLiterals) throw new Error('RomFS recipe does not match the literal data.');
  return gaps;
}

function sortedTextureWrites(desc,source,index,packLength) {
  // Pack each offset and RGB5551 word into one exact 53-bit-safe number.
  // Sorting typed scalars avoids allocating one JS array per texel.
  const writes=resolveTextures(desc,source,index,packLength);
  const packed=new Float64Array(writes.size);
  let i=0;
  for(const [offset,value] of writes)packed[i++]=offset*65536+value;
  return packed.sort();
}

export async function build3dsx(rom, manifest, loadPart, progress = () => {}) {
  if (manifest?.schema === 11) {
    const m = validateManifest(manifest);
    const base = await build3dsx(rom, m.base_build, loadPart, progress);
    progress('Applying the current game update...');
    const descriptor = m.parts['runtime-update.bps.gz'];
    const compressed = await loadPart('runtime-update.bps.gz');
    if (compressed.byteLength !== descriptor.size || await sha256(compressed) !== descriptor.sha256) {
      throw Error('Runtime update input verification failed.');
    }
    const bytes = new Uint8Array(descriptor.decoded_size);
    let at = 0;
    await inflate(compressed, bytes.length, block => { bytes.set(block, at); at += block.length; });
    if (await sha256(bytes) !== m.runtime_patch_sha256) throw Error('Runtime patch verification failed.');
    if (typeof globalThis.BPS !== 'function' || typeof globalThis.BinFile !== 'function') {
      throw Error('The BPS runtime is unavailable. Reload the patcher.');
    }
    const patch = globalThis.BPS.fromFile(new globalThis.BinFile(bytes));
    if (patch.sourceSize !== base.length || patch.targetSize !== m.output.size) {
      throw Error('Runtime patch sizes differ.');
    }
    const output = patch.apply(new globalThis.BinFile(base), true)._u8array;
    if (output.length !== m.output.size || await sha256(output) !== m.output.sha256) {
      throw Error('Output verification failed. No file was downloaded.');
    }
    return output;
  }

  const m = validateManifest(manifest);
  const index = await detectROM(rom, m);
  const source = new Uint8Array(rom);
  async function verified(name) {
    const bytes = await loadPart(name);
    if (bytes.byteLength !== m.parts[name].size || await sha256(bytes) !== m.parts[name].sha256) {
      throw new Error('Build input verification failed. Reload the patcher and try again.');
    }
    return bytes;
  }

  let patchedGame;
  if (m.schema === 10) {
    progress('Preparing verified game assets locally...');
    const name = gamePatchNames[index];
    const encoded = await verified(name);
    const patch = new Uint8Array(m.parts[name].decoded_size);
    await inflate(encoded, patch.length, (chunk, offset) => patch.set(chunk, offset));
    patchedGame = await applyApprovedGamePatch(source, patch, {
      patch_sha256: m.game_patch.patch_sha256[index], target_sha256: m.game_patch.target_sha256,
      target_size: m.game_patch.target_size
    });
  }
  progress('Loading the 3DS build...');
  const recipeLauncher = [4, 5, 6, 7, 10].includes(m.schema);
  const components = recipeLauncher ? [...files, launcherRecipeName] : files;
  if ([6,7].includes(m?.schema)) components.push('outdoor-tiles.gz');
  if (m.schema === 10) components.push(gameRecipeName);
  const loaded = await Promise.all(components.map(verified));
  const [launcher, compressedRecipe, literals, texturePart, launcherRecipe] = loaded;
  const outdoorTiles = [6,7].includes(m.schema) ? loaded[5] : null;
  const gameReferences = m.schema === 10 ? loaded[5] : null;
  const prefixSize = recipeLauncher ? m.launcher_size : m.parts['launcher.gz'].decoded_size;
  const output = new Uint8Array(m.output.size);
  const pack = output.subarray(prefixSize);
  const recipe = new Uint8Array(m.parts['romfs-recipe.gz'].decoded_size);
  progress('Assembling your 3DS game...');
  await inflate(compressedRecipe, recipe.length, (chunk, offset) => recipe.set(chunk, offset));
  const textureData=new Uint8Array(m.parts['romfs-textures.gz'].decoded_size);
  await inflate(texturePart,textureData.length,(chunk,offset)=>textureData.set(chunk,offset));
  const textureRecipe = JSON.parse(new TextDecoder().decode(textureData));
  if (m.schema < 7 && Array.isArray(textureRecipe?.operations) && textureRecipe.operations.length > 24576) {
    throw Error('Invalid texture recipe');
  }
  const writes=sortedTextureWrites(textureRecipe,source,index,pack.length);
  const gaps = applyReferences(recipe, source, index, pack, m.romfs.literal_size,writes);
  const buildLauncher = async () => {
    if (!recipeLauncher) {
      await inflate(launcher, prefixSize, (chunk, offset) => output.set(chunk, offset));
      return;
    }
    const prefix = output.subarray(0, prefixSize);
    const rows = new Uint8Array(m.parts[launcherRecipeName].decoded_size);
    await inflate(launcherRecipe, rows.length, (chunk, offset) => rows.set(chunk, offset));
    const size = m.parts['launcher.gz'].decoded_size;
    let fontWrites = [];
    if (m.schema >= 5) {
      const font = rebuildShortFont(m.short_font, source, index, prefix.length);
      fontWrites = new Float64Array(font.length / 2);
      for (let word = 0; word < fontWrites.length; word++) {
        fontWrites[word] = (m.short_font.destination + word * 2) * 65536
          + font[word * 2] + font[word * 2 + 1] * 256;
      }
    }
    if ([6,7,10].includes(m.schema)) {
      if ((m.schema !== 10 && m.parts['outdoor-tiles.gz'].decoded_size > 4096)
          || !Array.isArray(m.outdoor_graphics) || m.outdoor_graphics.length > 64) {
        throw Error('Invalid outdoor graphics recipe.');
      }
      let pool;
      if (m.schema === 10) {
        pool = patchedGame.subarray(m.game_patch.pool_offset, m.game_patch.pool_offset + m.game_patch.pool_size);
      } else {
        pool = new Uint8Array(m.parts['outdoor-tiles.gz'].decoded_size);
        await inflate(outdoorTiles, pool.length, (chunk, offset) => pool.set(chunk, offset));
      }
      const generated = Array.from(fontWrites);
      for (const bank of m.outdoor_graphics) {
        const graphics = rebuildOutdoorBank(bank, source, index, pool);
        if (!integer(bank.offset, 0, prefix.length - graphics.length) || bank.offset % 2) {
          throw Error('Outdoor graphics destination is out of bounds.');
        }
        for (let at = 0; at < graphics.length; at += 2) {
          generated.push((bank.offset + at) * 65536 + graphics[at] + graphics[at + 1] * 256);
        }
      }
      fontWrites = Float64Array.from(generated).sort();
    }
    const copiedGameBytes = m.schema === 10 ? m.game_reference_bytes : 0;
    let launcherGaps = applyReferences(rows, source, index, prefix, size + copiedGameBytes, fontWrites);
    if (m.schema === 10) {
      const recipe = new Uint8Array(m.parts[gameRecipeName].decoded_size);
      await inflate(gameReferences, recipe.length, (chunk, offset) => recipe.set(chunk, offset));
      launcherGaps = applyPatchedGameReferences(recipe, patchedGame, prefix, launcherGaps, size, copiedGameBytes);
    }
    await fillGaps(launcher, size, prefix, launcherGaps);
  };
  await Promise.all([buildLauncher(), fillGaps(literals, m.romfs.literal_size, pack, gaps)]);

  const header = new DataView(output.buffer, 0, 44);
  if (header.getUint32(0) !== 0x33445358 || header.getUint16(4, true) !== 44
      || header.getUint32(40, true) !== prefixSize) {
    throw new Error('Invalid 3DSX launcher header.');
  }
  progress('Checking the finished download...');
  // The complete digest covers the launcher and every byte of RomFS.
  if (await sha256(output) !== m.output.sha256) {
    throw new Error('Output verification failed. No file was downloaded.');
  }
  return output;
}
