const romSize = 16 * 1024 * 1024;
const hashPattern = /^[0-9a-f]{64}$/;
const integer = (value, min, max) => Number.isSafeInteger(value) && value >= min && value <= max;

async function sha256(bytes) {
  const digest = await crypto.subtle.digest('SHA-256', bytes);
  return Array.from(new Uint8Array(digest), value => value.toString(16).padStart(2, '0')).join('');
}

export async function applyApprovedGamePatch(source, patchBytes, expected) {
  const targetSize = expected?.target_size ?? romSize;
  if (!(source instanceof Uint8Array) || source.length !== romSize
      || !(patchBytes instanceof Uint8Array) || patchBytes.length < 16
      || patchBytes.length > 32 * 1024 * 1024 || !expected
      || !hashPattern.test(expected.patch_sha256) || !hashPattern.test(expected.target_sha256)
      || !integer(targetSize, 1, romSize)) {
    throw Error('Invalid approved game patch input.');
  }
  if (await sha256(patchBytes) !== expected.patch_sha256) throw Error('Game patch verification failed.');
  if (typeof globalThis.BPS !== 'function' || typeof globalThis.BinFile !== 'function') {
    throw Error('The BPS runtime is unavailable. Reload the patcher.');
  }
  const patch = globalThis.BPS.fromFile(new globalThis.BinFile(new Uint8Array(patchBytes)));
  if (patch.sourceSize !== romSize || patch.targetSize !== targetSize) throw Error('Invalid approved game patch sizes.');
  const patched = patch.apply(new globalThis.BinFile(new Uint8Array(source)), true)._u8array;
  if (patched.length !== targetSize || await sha256(patched) !== expected.target_sha256) {
    throw Error('Patched game verification failed.');
  }
  return patched;
}

export function applyPatchedGameReferences(recipe, game, destination, gaps, literalBytes, copiedBytes) {
  if (!(recipe instanceof Uint8Array) || recipe.length % 12 || recipe.length > 1024 * 1024
      || !(game instanceof Uint8Array) || !integer(game.length, 1, romSize)
      || !integer(literalBytes, 0, destination.length) || !integer(copiedBytes, 1, destination.length)) {
    throw Error('Invalid patched-game reference recipe.');
  }
  const table = new DataView(recipe.buffer, recipe.byteOffset, recipe.byteLength);
  const remaining = [];
  let gap = 0;
  let position = gaps.length ? gaps[0][0] : destination.length;
  let copied = 0;
  for (let row = 0; row < recipe.length; row += 12) {
    const offset = table.getUint32(row, true);
    const length = table.getUint32(row + 4, true);
    const source = table.getUint32(row + 8, true);
    if (!integer(offset, position, destination.length) || !integer(length, 1, destination.length - offset)
        || !integer(source, 0, game.length - length)) throw Error('Patched-game reference is out of bounds.');
    while (gap < gaps.length && offset >= gaps[gap][1]) {
      if (position < gaps[gap][1]) remaining.push([position, gaps[gap][1]]);
      gap++;
      position = gap < gaps.length ? gaps[gap][0] : destination.length;
    }
    if (gap >= gaps.length || offset < position || offset + length > gaps[gap][1]) {
      throw Error('Patched-game reference overlaps another component.');
    }
    if (position < offset) remaining.push([position, offset]);
    destination.set(game.subarray(source, source + length), offset);
    position = offset + length;
    copied += length;
  }
  while (gap < gaps.length) {
    if (position < gaps[gap][1]) remaining.push([position, gaps[gap][1]]);
    gap++;
    position = gap < gaps.length ? gaps[gap][0] : destination.length;
  }
  const retained = remaining.reduce((sum, [start, end]) => sum + end - start, 0);
  if (retained !== literalBytes || copied !== copiedBytes) throw Error('Patched-game recipe byte count differs.');
  return remaining;
}
