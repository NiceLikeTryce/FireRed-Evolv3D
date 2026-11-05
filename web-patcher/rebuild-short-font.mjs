// Reproduce the integrated short font from the player's FireRed normal font.
const fontBytes = 32768;
const glyphBytes = 64;

function packGlyph(pixels) {
  const output = new Uint8Array(glyphBytes);
  let cursor = 0;
  for (let tile = 0; tile < 4; tile++) {
    const x = (tile & 1) * 8;
    const y = (tile >> 1) * 8;
    for (let row = 0; row < 8; row++) {
      const at = (y + row) * 16 + x;
      for (const half of [4, 0]) {
        output[cursor++] = (pixels[at + half] << 6)
          | (pixels[at + half + 1] << 4)
          | (pixels[at + half + 2] << 2)
          | pixels[at + half + 3];
      }
    }
  }
  return output;
}

export function rebuildShortFont(recipe, rom, revision, launcherSize) {
  if (!recipe || recipe.kind !== 'firered-short-latin-v1'
      || ![0, 1].includes(revision)
      || !Array.isArray(recipe.sources) || recipe.sources.length !== 2
      || !recipe.sources.every(offset => Number.isSafeInteger(offset)
        && offset >= 0 && offset <= rom.length - fontBytes)
      || !Number.isSafeInteger(recipe.destination) || recipe.destination % 2
      || recipe.destination < 0 || recipe.destination > launcherSize - fontBytes) {
    throw Error('Short font reference is out of bounds.');
  }
  const source = recipe.sources[revision];
  const font = new Uint8Array(rom.subarray(source, source + fontBytes));

  // The short ligature lowers its top two rows and omits the intervening row.
  const ligature = 37 * glyphBytes;
  const firstRow = font.slice(ligature + 10, ligature + 12);
  const secondRow = font.slice(ligature + 12, ligature + 14);
  font.fill(255, ligature + 10, ligature + 12);
  font.set(firstRow, ligature + 12);
  font.set(secondRow, ligature + 14);

  // Clear the plus sign's otherwise unused top row.
  font.fill(0, 46 * glyphBytes, 46 * glyphBytes + 2);

  // The unused character is a five-pixel outline with a one-pixel shadow.
  const box = new Uint8Array(16 * 16);
  for (let y = 0; y < 13; y++) {
    for (let x = 0; x < 5; x++) {
      box[y * 16 + x] = y === 0 || y === 12 || x === 0 || x === 4 ? 1 : 3;
    }
    box[y * 16 + 5] = 2;
  }
  box.fill(2, 13 * 16, 13 * 16 + 6);
  font.set(packGlyph(box), 80 * glyphBytes);
  return font;
}
